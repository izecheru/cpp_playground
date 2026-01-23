#pragma once
#include <entt/entt.hpp>
#include "registry.hpp"

struct IdentifierComponent
{
  std::string name{};
};

class Entity
{
public:
  explicit Entity( Registry& registry, const std::string& name )
      : m_entity{ registry.createEntity() }
      , m_registry{ &registry }
      , m_name{ name }
  {
  }

  Entity( const Entity& other )
      : m_entity{ other.m_entity }
      , m_registry{ other.m_registry }
      , m_name{ other.m_name }
  {
  }

  Entity( Entity&& other ) noexcept
      : m_entity{ other.m_entity }
      , m_registry{ other.m_registry }
      , m_name{ other.m_name }
  {
    other.m_entity = entt::null;
    other.m_registry = nullptr;
    other.m_name = "";
  }

  Entity& operator=( const Entity& other )
  {
    if ( this != &other )
    {
      this->m_entity = other.getEntityId();
      this->m_registry = other.m_registry;
      this->m_name = other.m_name;
    }

    return *this;
  }

  Entity& operator=( Entity&& other ) noexcept
  {
    if ( this != &other )
    {
    }

    return *this;
  }

  ~Entity() = default;

  template <typename TComponent, typename... Args>
  auto addComponent( entt::entity entity, Args&&... args ) -> TComponent&
  {
    return m_registry->addComponent<TComponent>( entity, std::forward<Args>( args )... );
  }

  template <typename TComponent>
  bool hasComponent( entt::entity entity )
  {
    return m_registry->hasComponent<TComponent>( entity );
  }

  template <typename TComponent>
  inline auto getComponent( entt::entity entity ) -> TComponent&
  {
    return m_registry->getComponent<TComponent>( entity );
  }

  template <typename TComponent, typename... Args>
  inline auto emplaceComponent( entt::entity entity, Args&&... args ) -> TComponent&
  {
    return m_registry->emplaceComponent<TComponent>( entity, std::forward<Args>( args )... );
  }

  template <typename TComponent, typename... Args>
  inline void removeComponent( entt::entity entity, Args&&... args )
  {
    m_registry->removeComponent<TComponent>( entity, std::forward<Args>( args )... );
  }

  inline auto getEntityId() const -> entt::entity
  {
    return m_entity;
  }

  inline auto getEntityName() const -> std::string
  {
    return m_name;
  }

  inline auto getRegistry() -> Registry*
  {
    return m_registry;
  }

private:
  entt::entity m_entity;
  std::string m_name;
  Registry* m_registry;
};