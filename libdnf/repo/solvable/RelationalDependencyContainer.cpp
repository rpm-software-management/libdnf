#include "RelationalDependencyContainer.hpp"
#include "Dependency.hpp"

#ifdef __cplusplus
extern "C" {
#endif

RelationalDependencyContainer::RelationalDependencyContainer()
        : sack(nullptr)
{
    queue_init(&queue);
}

RelationalDependencyContainer::RelationalDependencyContainer(const RelationalDependencyContainer &src)
        : sack(src.sack)
{
    queue_init_clone(&this->queue, &queue);
}


RelationalDependencyContainer::RelationalDependencyContainer(DnfSack *sack)
        : sack(sack)
{
    queue_init(&queue);
}

RelationalDependencyContainer::RelationalDependencyContainer(DnfSack *sack, Queue queue)
        : sack(sack)
{
    queue_init_clone(&this->queue, &queue);
}

RelationalDependencyContainer::~RelationalDependencyContainer()
{
    queue_free(&queue);
}

RelationalDependencyContainer &RelationalDependencyContainer::operator=(RelationalDependencyContainer &&src) noexcept
{
    sack = src.sack;
    queue_init_clone(&queue, &src.queue);
    return *this;
}

bool RelationalDependencyContainer::operator!=(const RelationalDependencyContainer &r) const { return !(*this == r); }
bool RelationalDependencyContainer::operator==(const RelationalDependencyContainer &r) const
{
    if (queue.count != r.queue.count)
        return false;

    for (int i = 0; i < queue.count; i++) {
        if (queue.elements[i] != r.queue.elements[i]) {
            return false;
        }
    }

    return dnf_sack_get_pool(sack) == dnf_sack_get_pool(r.sack);
}

void RelationalDependencyContainer::add(SolvableDependency *dependency)
{
    queue_push(&queue, dependency->getId());
}

void RelationalDependencyContainer::extend(RelationalDependencyContainer *container)
{
    queue_insertn(&queue, 0, container->queue.count, container->queue.elements);
}

std::shared_ptr<SolvableDependency> RelationalDependencyContainer::get(int index) const noexcept
{
    Id id = queue.elements[index];
    return std::make_shared<SolvableDependency>(sack, id);
}

SolvableDependency *RelationalDependencyContainer::getPtr(int index) const noexcept
{
    Id id = queue.elements[index];
    return new SolvableDependency(sack, id);
}

int RelationalDependencyContainer::count() const noexcept { return queue.count; }
const Pool *RelationalDependencyContainer::getPool() const noexcept { return dnf_sack_get_pool(sack); }
const Queue &RelationalDependencyContainer::getQueue() const noexcept { return queue; }

#ifdef __cplusplus
};
#endif