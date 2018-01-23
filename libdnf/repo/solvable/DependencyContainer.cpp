#include "DependencyContainer.hpp"
#include "Dependency.hpp"

DependencyContainer::DependencyContainer()
        : sack(nullptr)
{
    queue_init(&queue);
}

DependencyContainer::DependencyContainer(const DependencyContainer &src)
        : sack(src.sack)
{
    queue_init_clone(&this->queue, &queue);
}


DependencyContainer::DependencyContainer(DnfSack *sack)
        : sack(sack)
{
    queue_init(&queue);
}

DependencyContainer::DependencyContainer(DnfSack *sack, Queue queue)
        : sack(sack)
{
    queue_init_clone(&this->queue, &queue);
}

DependencyContainer::~DependencyContainer()
{
    queue_free(&queue);
}

DependencyContainer &DependencyContainer::operator=(DependencyContainer &&src) noexcept
{
    sack = src.sack;
    queue_init_clone(&queue, &src.queue);
    return *this;
}

bool DependencyContainer::operator!=(const DependencyContainer &r) const { return !(*this == r); }
bool DependencyContainer::operator==(const DependencyContainer &r) const
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

void DependencyContainer::add(Dependency *dependency)
{
    queue_push(&queue, dependency->getId());
}

void DependencyContainer::extend(DependencyContainer *container)
{
    queue_insertn(&queue, 0, container->queue.count, container->queue.elements);
}

std::shared_ptr<Dependency> DependencyContainer::get(int index) const noexcept
{
    Id id = queue.elements[index];
    return std::make_shared<Dependency>(sack, id);
}

Dependency *DependencyContainer::getPtr(int index) const noexcept
{
    Id id = queue.elements[index];
    return new Dependency(sack, id);
}

int DependencyContainer::count() const noexcept { return queue.count; }
const Queue &DependencyContainer::getQueue() const noexcept { return queue; }
