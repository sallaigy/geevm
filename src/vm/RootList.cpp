#include "vm/GarbageCollector.h"

using namespace geevm;

RootList::Node* RootList::insert(Instance* reference)
{
  Node* newNode = new Node(reference, mHead, nullptr);
  if (mHead != nullptr) {
    mHead->prev = newNode;
  }
  mHead = newNode;

  return newNode;
}

void RootList::remove(Node* node)
{
  if (node == nullptr) {
    return;
  }

  if (node->next != nullptr) {
    node->next->prev = node->prev;
  }

  if (node->prev != nullptr) {
    node->prev->next = node->next;
  }

  if (node == mHead) {
    mHead = node->next;
  }

  delete node;
}

RootList::~RootList()
{
  Node* node = mHead;
  while (node != nullptr) {
    Node* next = node->next;
    delete node;
    node = next;
  }
}
