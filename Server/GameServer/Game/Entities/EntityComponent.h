#pragma once

/**
 * 엔티티의 기능 하나를 맡는 컴포넌트의 베이스.
 *
 * 엔티티가 기능별 컴포넌트를 직접 소유하고, 자기 생명주기와 스레드를 그대로 나눠 준다.
 * 컴포넌트는 소유자의 수명을 붙잡지 않는다.
 */
class EntityComponent
{
public:
    explicit EntityComponent(EntityRef owner) : _owner(owner) {}
    virtual ~EntityComponent() = default;

    /** 소유자의 Start에서 한 번 부른다. */
    virtual void Start() {}
    /** 소유자의 Tick에서 부른다. 틱을 돌리지 않는 소유자는 부르지 않는다. */
    virtual void Tick(float deltaTime) {}

protected:
    weak_ptr<Entity> _owner;
};
