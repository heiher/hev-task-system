/* Check that pointer packing does not assume MSVC's 32-bit 'long' on Win64. */
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include "lib/rbtree/hev-rbtree.h"

#if defined(_MSC_VER)
_Static_assert (sizeof (((HevRBTreeNode *)0)->__parent_color) == sizeof (uintptr_t),
                "MSVC rbtree parent must be pointer-sized");
#else
_Static_assert (sizeof (((HevRBTreeNode *)0)->__parent_color) == sizeof (unsigned long),
                "GNU rbtree parent must retain its original ABI");
#endif

#if defined(_M_ARM64) && !defined(_M_ARM64EC)
/* Native Windows ARM64 is LLP64: 64-bit pointers but 32-bit 'long'. */
_Static_assert (sizeof (void *) == 8, "ARM64 pointers must be 64-bit");
_Static_assert (sizeof (long) == 4, "ARM64 Windows 'long' must be 32-bit");
_Static_assert (sizeof (uintptr_t) == 8, "ARM64 uintptr_t must be 64-bit");
#endif

int main (void)
{
    HevRBTree tree = { 0 };
    HevRBTreeNode node;
    hev_rbtree_node_link (&node, NULL, &tree.root);
    assert (hev_rbtree_node_parent (&node) == NULL);
    return 0;
}
