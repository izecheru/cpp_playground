#pragma once
#include <entt/entt.hpp>

class Registry
{
public:
  Registry()
      : m_registry{ std::make_shared<entt::registry>() }
  {
  }

  ~Registry() = default;

  auto createEntity() -> entt::entity
  {
    return m_registry->create();
  }

  void removeEntity( entt::entity entity )
  {
    m_registry->destroy( entity );
  }

  template <typename TComponent, typename... Args>
  auto addComponent( entt::entity entity, Args&&... args ) -> TComponent&
  {
    return m_registry->emplace<TComponent>( entity, std::forward<Args>( args )... );
  }

  template <typename TComponent>
  bool hasComponent( entt::entity entity )
  {
    return m_registry->any_of<TComponent>( entity );
  }

  template <typename TComponent>
  inline auto getComponent( entt::entity entity ) -> TComponent&
  {
    return m_registry->get<TComponent>( entity );
  }

  template <typename TComponent, typename... Args>
  inline auto emplaceComponent( entt::entity entity, Args&&... args ) -> TComponent&
  {
    return m_registry->emplace_or_replace<TComponent>( entity, std::forward<Args>( args )... );
  }

  template <typename TComponent>
  inline void removeComponent( entt::entity entity )
  {
    m_registry->remove<TComponent>( entity );
  }

  auto storage()
  {
    return m_registry->storage();
  }

  auto getEnttRegistry() -> entt::registry*
  {
    return m_registry.get();
  }

private:
  std::shared_ptr<entt::registry> m_registry;
};