#ifndef _HPP_LOADER
#define _HPP_LOADER

namespace blunted {

// loads specific kind of resource

template <typename T>
class Resource;

template <typename T>
class Loader {
public:
  virtual void Load(const std::string& filename, boost::intrusive_ptr<Resource<T>> resource) = 0;

protected:
};

}  // namespace blunted

#endif
