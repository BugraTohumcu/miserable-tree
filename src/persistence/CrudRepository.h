#ifndef CRUD_REPO
#define CRUD_REPO


namespace mislib
{
    template<typename T>
    class CrudRepository{
    protected:
    virtual bool create( const T& data ) = 0;
    // virtual bool get( size_t id, T& out ) = 0;
    // virtual bool update( size_t id, T& newData ) = 0;
    // virtual bool remove( size_t id ) = 0;

    ~CrudRepository() = default;
};
    
} // namespace mislib


#endif