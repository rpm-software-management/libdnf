#ifndef LIBDNF_RELATIONALDEPENDENCYCONTAINER_HPP
#define LIBDNF_RELATIONALDEPENDENCYCONTAINER_HPP


#include <solv/queue.h>
#include <memory>
#include "libdnf/dnf-sack.h"

struct SolvableDependency;

struct RelationalDependencyContainer
{
public:
    RelationalDependencyContainer();
    RelationalDependencyContainer(const RelationalDependencyContainer &src);

    explicit RelationalDependencyContainer(DnfSack *sack);
    RelationalDependencyContainer(DnfSack *sack, Queue queue);
    ~RelationalDependencyContainer();

    RelationalDependencyContainer &operator=(RelationalDependencyContainer &&src) noexcept;
    bool operator==(const RelationalDependencyContainer &r) const;
    bool operator!=(const RelationalDependencyContainer &r) const;

    void add(SolvableDependency *dependency);
    void extend(RelationalDependencyContainer *container);

    std::shared_ptr<SolvableDependency> get(int index) const noexcept;
    SolvableDependency *getPtr(int index) const noexcept;
    int count() const noexcept;

    const Pool *getPool() const noexcept;
    const Queue &getQueue() const noexcept;

private:
    DnfSack *sack;
    Queue queue;
};

#endif //LIBDNF_RELATIONALDEPENDENCYCONTAINER_HPP
