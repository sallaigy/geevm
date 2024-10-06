#ifndef GEEVM_VM_INSTANCE_H
#define GEEVM_VM_INSTANCE_H

namespace geevm
{

class JClass;

class Instance
{
public:
  explicit Instance(JClass* klass)
    : mClass(klass)
  {
  }

  JClass* getClass() const
  {
    return mClass;
  }

private:
  JClass* mClass;
};

} // namespace geevm

#endif // GEEVM_VM_INSTANCE_H
