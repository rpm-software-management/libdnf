#ifndef LIBDNF_DEPENDENCYCONTAINER_HPP
#define LIBDNF_DEPENDENCYCONTAINER_HPP


#include <solv/queue.h>
#include <memory>
#include "libdnf/dnf-sack.h"

struct Dependency;

struct DependencyContainer
{
public:
    DependencyContainer();
    DependencyContainer(const DependencyContainer &src);

    explicit DependencyContainer(DnfSack *sack);
    DependencyContainer(DnfSack *sack, Queue queue);
    ~DependencyContainer();

    DependencyContainer &operator=(DependencyContainer &&src) noexcept;
    bool operator==(const DependencyContainer &r) const;
    bool operator!=(const DependencyContainer &r) const;

    void add(Dependency *dependency);
    void extend(DependencyContainer *container);

    std::shared_ptr<Dependency> get(int index) const noexcept;
    Dependency *getPtr(int index) const noexcept;
    int count() const noexcept;

    const Queue &getQueue() const noexcept;

private:
    DnfSack *sack;
    Queue queue;
};

#endif //LIBDNF_DEPENDENCYCONTAINER_HPP
