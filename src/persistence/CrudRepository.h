#ifndef CRUD_REPO
#define CRUD_REPO

#include <cstdlib>
namespace mislib
{
    template<typename T>
    class CrudRepository{
    protected:
    virtual bool create( const T& data ) = 0;
    virtual bool get( std::size_t id, T& out ) = 0;
    virtual bool update( const T& data ) = 0;
    virtual bool remove( std::size_t id ) = 0;

    ~CrudRepository() = default;
};
    
} // namespace mislib


#endif