#define SOL_ALL_SAFETIES_ON 1
#include <array>
#include <entt/entt.hpp>
#include <entt/meta/factory.hpp> // Needed for the .type<T>() factory
#include <entt/meta/meta.hpp>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <sol/sol.hpp>
#include <string>
#include "entity.hpp"
#include "event_dispatcher.hpp"
#include "registry.hpp"
using namespace entt::literals;

[[nodiscard]] auto getTypeId( const sol::table& obj ) -> entt::id_type
{
  // be careful for this one to be written exactly the way you store it
  const auto f = obj["typeId"].get<sol::function>();
  assert( f.valid() && "type_id not exposed to lua!" );
  return f.valid() ? f().get<entt::id_type>() : -1;
}

template <typename TEvent>
[[nodiscard]] auto deduceType( TEvent&& obj ) -> entt::id_type
{
  switch ( obj.get_type() )
  {
  // if we have registry:has(entity, Transform.type_id()) in lua file the it is a number
  case sol::type::number:
    return obj.template as<entt::id_type>();
  // if we have registry:has(entity, Transform) then we have a table, like struct {}
  case sol::type::table:
    return getTypeId( obj );
  }
  assert( false );
  return -1;
}

// https://github.com/skypjack/entt/wiki/Crash-Course:-runtime-reflection-system
template <typename... Args>
inline auto invokeMetaFunc( entt::meta_type meta_type, entt::id_type function_id, Args&&... args ) -> entt::meta_any
{
  if ( !meta_type )
  {
    std::cout << "meta type is not registered in entt::meta";
  }
  else
  {
    if ( auto&& metaFunction = meta_type.func( function_id ); metaFunction )
      return metaFunction.invoke( {}, std::forward<Args>( args )... );
  }
  return entt::meta_any{};
}

template <typename... Args>
inline auto invokeMetaFunc( entt::id_type type_id, entt::id_type function_id, Args&&... args ) -> entt::meta_any
{
  return invokeMetaFunc( entt::resolve( type_id ), function_id, std::forward<Args>( args )... );
}

template <typename TComponent>
bool hasComponent( Registry& registry, entt::entity entity )
{
  return registry.hasComponent<TComponent>( entity );
}

/**
 * @brief Adds a component to an entity in the regsitry
 * @tparam TComponent Type of component we add
 * @param registry The registry we currently modify
 * @param entity The entity we add the component to
 * @param comp The lua component
 * @param currentState Lua state NEEDED FOR MAKE REFERENCE!!!
 * @return A reference to the created component
 */
template <typename TComponent>
auto addComponent( Registry& registry, entt::entity entity, const sol::table& comp, sol::this_state currentState )
{
  auto& component =
    registry.addComponent<TComponent>( entity, comp.valid() ? std::move( comp.as<TComponent&&>() ) : TComponent{} );
  // this is what we need the current lua state for
  return sol::make_reference( currentState, std::ref( component ) );
}

template <typename TComponent>
auto getComponent( Registry& registry, entt::entity entity, sol::this_state currentState )
{
  auto& component = registry.getComponent<TComponent>( entity );
  return sol::make_reference( currentState, std::ref( component ) );
}

template <typename TComponent>
auto emplaceComponent( Registry& registry, entt::entity entity, const sol::table& comp, sol::this_state currentState )
{
  auto& component =
    registry.emplaceComponent<TComponent>( entity, comp.valid() ? std::move( comp.as<TComponent&&>() ) : TComponent{} );

  // this is what we need the current lua state for
  return sol::make_reference( currentState, std::ref( component ) );
}

template <typename TComponent>
auto removeComponent( Registry& registry, entt::entity entity )
{
  registry.removeComponent<TComponent>( entity );
}

template <typename TComponent>
void registerMetaComponent()
{
  entt::meta_factory<TComponent>()
    .type( entt::type_hash<TComponent>::value() )
    .template func<&addComponent<TComponent>>( "add"_hs )
    .template func<&getComponent<TComponent>>( "get"_hs )
    .template func<&hasComponent<TComponent>>( "has"_hs )
    .template func<&removeComponent<TComponent>>( "remove"_hs )
    .template func<&emplaceComponent<TComponent>>( "emplace"_hs );
}

template <typename Event>
auto add_handler( EventDispatcher& dispatcher, const sol::function& f )
{
  return std::make_unique<LuaEventHandler<Event>>( dispatcher, f );
}

template <typename Event>
auto add_handler2( EventDispatcher& dispatcher, const sol::object& func )
{
  if ( !func.valid() )
  {
    std::cout << "invalid func\n";
    return;
  }

  auto* funcRef{ func.as<LuaHandler<Event>*>() };
  funcRef->connection = dispatcher.addHandler<Event, &LuaHandler<Event>::handle>( *funcRef );
}

template <typename Event>
void dispatch_event( EventDispatcher& dispatcher, const sol::table& evt )
{
  dispatcher.dispatchEvent( evt.as<Event>() );
}

template <typename TEvent>
bool has_handlers( EventDispatcher& dispatcher )
{
  return dispatcher.hasHandlers<TEvent>();
}

template <typename TEvent>
void registerMetaEvent()
{
  entt::meta_factory<TEvent>()
    .type( entt::type_hash<TEvent>::value() )
    .template func<&add_handler<TEvent>>( "add_handler"_hs )
    .template func<&add_handler2<TEvent>>( "add_handler2"_hs )
    .template func<&has_handlers<TEvent>>( "has_handlers"_hs )
    .template func<&dispatch_event<TEvent>>( "dispatch_event"_hs );
}

struct FileComponent
{
  std::string filePath;
};

struct IndexComponent
{
  uint32_t index{ 0 };
};

struct TransformComponent
{
  glm::vec3 pos{};
  glm::vec3 rotation{};
  glm::vec3 scale{};
};

auto collectTypes( const sol::variadic_args& va )
{
  std::set<entt::id_type> types;
  std::transform( va.cbegin(), va.cend(), std::inserter( types, types.begin() ), []( const auto& obj ) {
    return deduceType( obj );
  } );
  return types;
}

template <typename TEvent, typename TData>
TData getField( entt::meta_any& object, const char* fieldName )
{
  auto type = entt::resolve<TEvent>();
  auto field = type.data( entt::hashed_string::value( fieldName ) );

  if ( !field )
    throw std::runtime_error( "Field not found" );

  return field.get( object ).cast<TData>();
}

struct LuaEvent
{
  sol::object data;
};

auto main() -> int
{
  sol::state lua;
  lua.open_libraries( sol::lib::base, sol::lib::package, sol::lib::string );

  registerMetaComponent<FileComponent>();
  registerMetaComponent<TransformComponent>();
  registerMetaComponent<IndexComponent>();

  lua.new_usertype<FileComponent>(
    "FileComponent",
    "typeId",
    &entt::type_hash<FileComponent>::value,
    sol::call_constructor,
    sol::factories( []( const std::string& path ) { return FileComponent{ .filePath = path }; } ),
    "filePath",
    &FileComponent::filePath );

  lua.new_usertype<IndexComponent>(
    "IndexComponent",
    "typeId",
    &entt::type_hash<IndexComponent>::value,
    sol::call_constructor,
    sol::factories( []( const uint32_t index ) { return IndexComponent{ .index = index }; } ),
    "index",
    &IndexComponent::index );

  lua.new_usertype<ScriptEvent>(
    "ScriptEvent",
    sol::call_constructor,
    sol::factories( []() { return ScriptEvent{}; }, []( sol::table data ) { return ScriptEvent{ .data = data }; } ),
    "typeId",
    &entt::type_hash<ScriptEvent>::value,
    "data",
    &ScriptEvent::data );

  lua.new_usertype<LuaHandler<LuaEvent>>(
    "LuaEventHandler",
    "typeId",
    entt::type_hash<LuaHandler<LuaEvent>>::value,
    "eventType",
    entt::type_hash<LuaEvent>::value,
    sol::call_constructor,
    sol::factories( []( const sol::function& func ) { return LuaHandler<LuaEvent>{ .callback = func }; } ),
    "release",
    &LuaHandler<LuaEvent>::release );

  lua.new_usertype<LuaEvent>( "LuaEvent",
                              "typeId",
                              &entt::type_hash<LuaEvent>::value,
                              sol::call_constructor,
                              sol::factories( []( const sol::object& data ) { return LuaEvent{ .data = data }; } ),
                              "data",
                              &LuaEvent::data );

  lua.new_usertype<MessageEvent>(
    "MessageEvent",
    sol::call_constructor,
    sol::factories( []() { return MessageEvent{ "empty" }; }, // Default constructor
                    []( const std::string& message ) { return MessageEvent{ .message = message }; } // Data constructor
                    ),
    "typeId",
    &entt::type_hash<MessageEvent>::value,
    "message",
    &MessageEvent::message );

  EventDispatcher dispatcher{};
  lua.new_usertype<EventDispatcher>(
    "EventDispatcher",
    sol::call_constructor,
    sol::factories( [&]( bool lua ) {
      // if lua == true then the dispatcher is local to lua
      if ( lua )
        return EventDispatcher{};

      // this is a reference
      return dispatcher;
    } ),
    "hasHandlers",
    []( EventDispatcher& self, const sol::table& eventTypeOrId ) {
      if ( getTypeId( eventTypeOrId ) == entt::type_hash<ScriptEvent>::value() )
      {
        auto any = invokeMetaFunc( deduceType( eventTypeOrId ), "has_handlers"_hs, self );
        return any ? any.cast<bool>() : false;
      }
      return false;
    },
    "addHandler",
    []( EventDispatcher& self, const sol::object& eventTypeOrId, const sol::object& listener, sol::this_state s ) {
      if ( !listener.valid() )
      {
        return entt::meta_any{};
      }
      return invokeMetaFunc( getTypeId( eventTypeOrId ), "add_handler"_hs, self, listener );
    },
    "addHandlerExperimental",
    []( EventDispatcher& self, const sol::object& eventTypeOrId, const sol::object& listener, sol::this_state s ) {
      if ( !listener.valid() )
      {
        return entt::meta_any{};
      }
      return invokeMetaFunc( getTypeId( eventTypeOrId ), "add_handler2"_hs, self, listener );
    },
    "dispatchEvent",
    []( EventDispatcher& self, const sol::table& event ) {
      const auto eventId = getTypeId( event );

      if ( eventId == entt::type_hash<ScriptEvent>::value() )
      {
        // this should go only to the lua side
        self.dispatchEvent<ScriptEvent>( ScriptEvent{ event } );
      }
      else
      {
        // this goes to lua as well as c++
        invokeMetaFunc( eventId, "dispatch_event"_hs, self, event );
      }
    } );

  registerMetaEvent<ScriptEvent>();
  registerMetaEvent<MessageEvent>();
  registerMetaEvent<LuaEvent>();
  registerMetaEvent<LuaHandler<LuaEvent>>();

  lua.safe_script_file( "H:\\Git\\cpp_playground\\test.lua" );

  return 0;
}