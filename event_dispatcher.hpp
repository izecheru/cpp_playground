#pragma once
#include <entt/entt.hpp>
#include <memory>
#include <sol/sol.hpp>

struct ScriptEvent
{
  sol::table data;
};

struct MessageEvent
{
  std::string message;
};

class EventDispatcher
{
public:
  EventDispatcher()
      : m_pDispatcher{ std::make_shared<entt::dispatcher>() }
  {
  }

  ~EventDispatcher() = default;

  /**
   * @brief Ads a handler (listener) for a specific type of event
   * @tparam TEvent Event type we're listening for
   * @tparam TEventHandler The type of the instance that has the function handler for this type of event
   * @tparam Func Function reference that will get called once the event of said type is emitted
   * @param handler The *this pointer to the class that will process this type of event
   * @return
   */
  template <typename TEvent, auto Func, typename TEventHandler>
  auto addHandler( TEventHandler& handler )
  {
    return m_pDispatcher->sink<TEvent>().template connect<Func>( handler );
  }

  /**
   * @brief Checks to see if .sink<TEvent>().empty() is true
   * @tparam TEvent Type of event we're checking for
   * @param event Actual event
   * @return True if we don't have any listeners and false if we have
   */
  template <typename TEvent>
  bool hasHandlers()
  {
    return !m_pDispatcher->sink<TEvent>().empty();
  }

  /**
   * @brief Triggers a specific event
   * @tparam TEvent Type of event triggered
   * @param event Event information passed to the listeners
   * @return
   */
  template <typename TEvent>
  auto dispatchEvent( TEvent& event )
  {
    m_pDispatcher->trigger( event );
  }

  template <typename TEvent>
  auto dispatchEvent( TEvent&& event )
  {
    m_pDispatcher->trigger( std::forward<TEvent>( event ) );
  }

  /**
   * @brief Removes a specific listener from the list
   * @tparam TEvent Event type we're removing the listener for
   * @tparam THandler Type of class we're removing the listener from
   * @tparam Func The function reference we're removing
   * @param handler The *this pointer to the class that will process this type of event
   */
  template <typename TEvent, auto Func, typename THandler>
  void removeListener( THandler& handler )
  {
    m_pDispatcher->sink<TEvent>().template disconnect<Func>( handler );
  }

  /**
   * @brief Removes ALL listeners of a said TEvent from the list
   * @tparam TEvent Type of event we're removin all listeners for
   * @tparam THandler Type of class we're removing all the listeners from
   * @param handler The *this pointer to the class that will process this type of event
   */
  template <typename TEvent, typename THandler>
  void removeAllListeners( THandler& handler )
  {
    m_pDispatcher->sink<TEvent>().template disconnect( handler );
  }

  auto getDispatcher()
  {
    return m_pDispatcher;
  }

private:
  std::shared_ptr<entt::dispatcher> m_pDispatcher;
};

template <typename TEvent>
struct LuaEventHandler
{
  // the function reference from lua
  sol::function callback;

  // the connection made between lua handler and the handle<TEvent> function that calls the callback and executes the
  // actual code
  entt::connection connection;

  LuaEventHandler( EventDispatcher& dispatcher, const sol::function& func )
      : callback{ func }
      , connection{ dispatcher.addHandler<TEvent, &LuaEventHandler::handle>( *this ) }
  {
  }

  void release()
  {
    connection.release();
    callback.abandon();
  }

  void handle( const TEvent& event )
  {
    if ( connection && callback.valid() )
      callback( event );
  }
};
