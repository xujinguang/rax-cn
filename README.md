# Rax, an ANSI C radix tree implementation

Rax is a radix tree implementation initially written to be used in a specific
place of Redis in order to solve a performance problem, but immediately
converted into a stand alone project to make it reusable for Redis itself, outside the initial intended application, and for other projects as well.

Rax是一种基数树实现，最初被编写为在Redis的特定位置使用以解决性能问题，但立即转换为独立项目，
以使其可在Redis本身初始预期应用程序之外以及其他应用程序和项目中重用。

The primary goal was to find a suitable balance between performances
and memory usage, while providing a fully featured implementation of radix trees
that can cope with many different requirements.

基数树实现的主要目标是在性能和内存使用之间找到适当的平衡，同时提供齐全的功能以应付许多不同的需求。

During the development of this library, while getting more and more excited
about how practical and applicable radix trees are, I was very surprised to
see how hard it is to write a robust implementation, especially of a fully
featured radix tree with a flexible iterator. A lot of things can go wrong
in node splitting, merging, and various edge cases. For this reason a major
goal of the project is to provide a stable and battle tested implementation
for people to use and in order to share bug fixes. The project relies a lot
on fuzz testing techniques in order to explore not just all the lines of code
the project is composed of, but a large amount of possible states.

在这个库的开发过程中，当我对基数树的实用性和适用性越来越感兴趣的时候，我很惊讶地发现要编写
一个健壮的实现是多么的困难，特别是一个功能齐全的基数树和一个灵活的迭代器。在节点分割、合并
和各种边界情况下，很多事情都可能出错。出于这个原因，该项目的一个主要目标是提供一个稳定的、
经过实战考验的实现，供人们使用，并共享错误修复。该项目非常依赖模糊测试技术，以便不仅探索项目
组成的所有代码行，而且探索大量可能的状态。

Rax is an open source project, released under the BSD two clause license.

Rax是开源项目，基于BSD协议发布。

Major features:

* Memory conscious:
    + Packed nodes representation.
    + Able to avoid storing a NULL pointer inside the node if the key is set to NULL (there is an `isnull` bit in the node header).
    + Lack of parent node reference. A stack is used instead when needed.
* Fast lookups:
    + Edges are stored as arrays of bytes directly in the parent node, no need to access non useful children while trying to find a match. This translates into less cache misses compared to other implementations.
    + Cache line friendly scanning of the correct child by storing edges as two separated arrays: an array of edge chars and one of edge pointers.
* Complete implementation:
    + Deletion with nodes re-compression as needed.
    + Iterators (including a way to use iterators while the tree is modified).
    + Random walk iteration.
    + Ability to report and resist out of memory: if malloc() returns NULL the API can report an out of memory error and always leave the tree in a consistent state.
* Readable and fixable implementation:
    + All complex parts are commented with algorithms details.
    + Debugging messages can be enabled to understand what the implementation is doing when calling a given function.
    + Ability to print the radix tree nodes representation as ASCII art.
* Portable implementation:
    + Never does unaligned accesses to memory.
    + Written in ANSI C99, no extensions used.
* Extensive code and possible states test coverage using fuzz testing.
    + Testing relies a lot on fuzzing in order to explore non trivial states.
    + Implementation of the dictionary and iterator compared with behavior-equivalent implementations of simple hash tables and sorted arrays, generating random data and checking if the two implementations results match.
    + Out of memory condition tests. The implementation is fuzzed with a special allocator returning `NULL` at random. The resulting radix tree is tested for consistency. Redis, the primary target of this implementation, does not use this feature, but the ability to handle OOM may make this implementation useful where the ability to survive OOMs is needed.
    + Part of Redis: the implementation is stressed significantly in the real world.

The layout of a node is as follows. In the example, a node which represents
a key (so has a data pointer associated), has three children `x`, `y`, `z`.
Every space represents a byte in the diagram.

节点的布局如下。在这个例子中，一个表示一个键的节点(有一个数据指针关联)有三个子节 x，y，z，
每个空格表示图中的一个字节。
    +----+---+--------+--------+--------+--------+
    |HDR |xyz| x-ptr  | y-ptr  | z-ptr  |dataptr |
    +----+---+--------+--------+--------+--------+

The header `HDR` is actually a bitfield with the following fields:
标题 HDR 实际上是一个带有以下字段的位段:

    uint32_t iskey:1;     /* Does this node contain a key? */
    uint32_t isnull:1;    /* Associated value is NULL (don't store it). */
    uint32_t iscompr:1;   /* Node is compressed. */
    uint32_t size:29;     /* Number of children, or compressed string len. */

Compressed nodes represent chains of nodes that are not keys and have
exactly a single child, so instead of storing:
压缩节点表示非键节点的链，只有一个子节点，因此不以一下方式存储:

    A -> B -> C -> [some other node]

We store a compressed node in the form:
而是以下面的形式存储为一个压缩节点:

    "ABC" -> [some other node]

The layout of a compressed node is:
压缩节点的布局如下:

    +----+---+--------+
    |HDR |ABC|chld-ptr|
    +----+---+--------+

# Basic API

The basic API is a trivial dictionary where you can add or remove elements.
The only notable difference is that the insert and remove APIs also accept
an optional argument in order to return, by reference, the old value stored
at a key when it is updated (on insert) or removed.

基本 API 是一个简单的字典，您可以在其中添加或删除元素。唯一值得注意的区别是，插入和删除 api 
还接受一个可选参数，以便在更新(插入时)或删除时通过引用返回存储在键上的旧值。

## Creating a radix tree and adding a key

A new radix tree is created with:
创建一个新基数树

    rax *rt = raxNew();

In order to insert a new key, the following function is used:
插入一个新key，使用以下函数

    int raxInsert(rax *rax, unsigned char *s, size_t len, void *data,
                  void **old);

Example usage:
示例

    raxInsert(rt,(unsigned char*)"mykey",5,some_void_value,NULL);

The function returns 1 if the key was inserted correctly, or 0 if the key
was already in the radix tree: in this case, the value is updated. The
value of 0 is also returned on out of memory, however in that case
`errno` is set to `ENOMEM`.

如果键被正确插入，函数返回1; 如果键已经在基数树中，则返回0: 在这种情况下，将更新值。
0的值也在内存不足的情况下返回，但是在这种情况下 errno 被设置为 ENOMEM。

If the associated value `data` is NULL, the node where the key
is stored does not use additional memory to store the NULL value, so
dictionaries composed of just keys are memory efficient if you use
NULL as associated value.

如果关联的值数据为 NULL，那么存储键的节点就不会使用额外的内存来存储 NULL 值，所以如果使用 
NULL 作为关联值，那么由键组成的字典就是全部有效的内存。

Note that keys are unsigned arrays of chars and you need to specify the
length: Rax is binary safe, so the key can be anything.

注意，key 是字符的无符号数组，您需要指定长度: Rax 是二进制安全的，因此 key 可以是任何值。

The insertion function is also available in a variant that will not
overwrite the existing key value if any:

插入函数也有一个变体，它不会覆盖现有的键值(如果有的话) :

    int raxTryInsert(rax *rax, unsigned char *s, size_t len, void *data,
                     void **old);

The function is exactly the same as raxInsert(), however if the key
exists the function returns 0 (like raxInsert) without touching the
old value. The old value can be still returned via the 'old' pointer
by reference.

该函数与 raxInsert ()完全相同，但是如果键存在，则函数返回0(如 raxInsert) ，而不修改旧值。
旧的值仍然可以通过引用返回“旧的”指针。


## Key lookup

The lookup function is the following:
查找函数如下:

    void *raxFind(rax *rax, unsigned char *s, size_t len);

This function returns the special value `raxNotFound` if the key you
are trying to access is not there, so an example usage is the following:

如果你试图访问的键不在，这个函数返回一个特殊的值 raxNotFound，用法如下

    void *data = raxFind(rax,mykey,mykey_len);
    if (data == raxNotFound) return;
    printf("Key value is %p\n", data);

raxFind() is a read only function so no out of memory conditions are
possible, the function never fails.

raxFind ()是一个只读函数，因此不会出现内存不足的情况，这个函数从来不会失败。

## Deleting keys

Deleting the key is as you could imagine it, but with the ability to
return by reference the value associated to the key we are about to
delete:

你可以按照自己预想方式删除键，并通过引用返回与我们要删除的键相关的值:

    int raxRemove(rax *rax, unsigned char *s, size_t len, void **old);

The function returns 1 if the key gets deleted, or 0 if the key was not
there. This function also does not fail for out of memory, however if
there is an out of memory condition while a key is being deleted, the
resulting tree nodes may not get re-compressed even if possible: the radix
tree may be less efficiently encoded in this case.

如果键被删除，函数返回1; 如果键不在，函数返回0。这个函数也不会因为内存不足而失败，但是如果在删除
键时出现内存不足的情况，那么即使可能，结果树节点也不会被重新压缩: 在这种情况下，基数树的编码效率
可能会降低。

The `old` argument is optional, if passed will be set to the key associated
value if the function successfully finds and removes the key.

old 参数是可选的，如果传递，那么如果函数成功地找到并删除了键，那么它将被设置为键的关联值。

# Iterators

The Rax key space is ordered lexicographically, using the value of the
bytes the keys are composed of in order to decide which key is greater
between two keys. If the prefix is the same, the longer key is considered
to be greater.

Rax 键空间按字母顺序排列，使用组成键的字节值来决定两个键之间哪个键更大。如果前缀相同，则认为长键更大

Rax iterators allow to seek a given element based on different operators
and then to navigate the key space calling `raxNext()` and `raxPrev()`.

Rax 迭代器允许根据不同的操作符寻找给定的元素，然后在调用 raxNext ()和 raxPrev ()的关键空间中巡航。

## Basic iterator usage

Iterators are normally declared as local variables allocated on the stack,
and then initialized with the `raxStart` function:

迭代器通常声明为栈上分配的局部变量，然后用 raxStart 函数初始化

    raxIterator iter;
    raxStart(&iter, rt); // Note that 'rt' is the radix tree pointer.

The function `raxStart` never fails and returns no value.
Once an iterator is initialized, it can be sought (sought is the past tens
of 'seek', which is not 'seeked', in case you wonder) in order to start
the iteration from the specified position. For this goal, the function
`raxSeek` is used:

函数 raxStart 从不失败，也不返回任何值。一旦一个迭代器被初始化，就可以从指定的位置开始迭代。
为了实现这个目标，使用 raxSeek 函数来指定迭代位置:

    int raxSeek(raxIterator *it, unsigned char *ele, size_t len, const char *op);

For instance one may want to seek the first element greater or equal to the
key `"foo"`:

例如，寻找第一个大于或等于键“ foo”的元素:

    raxSeek(&iter,">=",(unsigned char*)"foo",3);

The function raxSeek() returns 1 on success, or 0 on failure. Possible failures are:

函数 raxSeek ()成功返回1，失败返回0:

1. An invalid operator was passed as last argument.
2. An out of memory condition happened while seeking the iterator.

Once the iterator is sought, it is possible to iterate using the function
`raxNext` and `raxPrev` as in the following example:

一旦迭代器找到位置，就可以使用 raxNext 和 raxPrev 函数进行迭代，如下例所示

    while(raxNext(&iter)) {
        printf("Key: %.*s\n", (int)iter.key_len, (char*)iter.key);
    }

The function `raxNext` returns elements starting from the element sought
with `raxSeek`, till the final element of the tree. When there are no more
elements, 0 is returned, otherwise the function returns 1. However the function
may return 0 when an out of memory condition happens as well: while it attempts
to always use the stack, if the tree depth is large or the keys are big the
iterator starts to use heap allocated memory.

函数 raxNext 从使用 raxSeek 寻找的元素开始返回元素，直到树的最后一个元素。当没有更多的元素时，
返回0，否则函数返回1。然而，当内存不足的情况发生时，函数也可能返回0: 当它尝试总是使用堆栈时，
如果树深度很大或键很大，迭代器就开始使用堆分配的内存。

The function `raxPrev` works exactly in the same way, but will move towards
the first element of the radix tree instead of moving towards the last
element.

raxPrev 函数的工作方式完全相同，但是它将移动到基数树的第一个元素，而不是移动到最后一个元素。

# Releasing iterators

An iterator can be used multiple times, and can be sought again and again
using `raxSeek` without any need to call `raxStart` again. However, when the
iterator is not going to be used again, its memory must be reclaimed
with the following call:

迭代器可以被多次使用，并且可以使用 raxSeek 一次又一次地查找，而不需要再次调用 raxStart。
但是，当迭代器不再使用时，必须通过以下调用回收它的内存:

    raxStop(&iter);

Note that even if you do not call `raxStop`, most of the times you'll not
detect any memory leak, but this is just a side effect of how the
Rax implementation works: most of the times it will try to use the stack
allocated data structures. However for deep trees or large keys, heap memory
will be allocated, and failing to call `raxStop` will result into a memory
leak.

请注意，即使您不调用 raxStop，大多数时候您也不会检测到任何内存泄漏，但这只是 Rax 实现工作
方式的一个副作用: 大多数时候它会尝试使用堆栈分配的数据结构。然而，对于深度树或大键，将分配堆
内存，如果不调用 raxStop，将导致内存泄漏。


## Seek operators

The function `raxSeek` can seek different elements based on the operator.
For instance in the example above we used the following call:

raxSeek 函数可以根据运算符寻找不同的元素。例如，在上面的例子中，我们使用了以下调用:

    raxSeek(&iter,">=",(unsigned char*)"foo",3);

In order to seek the first element `>=` to the string `"foo"`. However
other operators are available. The first set are pretty obvious:

为了寻找字符串“ foo”的第一个元素 >= 。然而，其他运算符也可以使用，作用很明显

* `==` seek the element exactly equal to the given one.
* `>` seek the element immediately greater than the given one.
* `>=` seek the element equal, or immediately greater than the given one.
* `<` seek the element immediately smaller than the given one.
* `<=` seek the element equal, or immediately smaller than the given one.
* `^` seek the smallest element of the radix tree.
* `$` seek the greatest element of the radix tree.

When the last two operators, `^` or `$` are used, the key and key length
argument passed are completely ignored since they are not relevant.

当使用最后两个运算符 ^ 或 $时，传递的键和键长参数将被完全忽略，因为它们不相关。

Note how certain times the seek will be impossible, for example when the
radix tree contains no elements or when we are asking for a seek that is
not possible, like in the following case:

请注意某些时刻寻找是不可能的，例如当基数树不包含任何元素，或者当我们要求一个寻找是不可能的时候，
如下面的例子:

    raxSeek(&iter,">",(unsigned char*)"zzzzz",5);

We may not have any element greater than `"zzzzz"`. In this case, what
happens is that the first call to `raxNext` or `raxPrev` will simply return
zero, so no elements are iterated.

我们可能没有比“ zzzzz”更大的元素了。在这种情况下，对 raxNext 或 raxPrev 的第一个调用将简
单地返回零，因此不会迭代任何元素。

## Iterator stop condition

Sometimes we want to iterate specific ranges, for example from AAA to BBB.
In order to do so, we could seek and get the next element. However we need
to stop once the returned key is greater than BBB. The Rax library offers
the `raxCompare` function in order to avoid you need to code the same string
comparison function again and again based on the exact iteration you are
doing:

有时我们需要迭代特定的范围，例如从 AAA 到 BBB。为了做到这一点，我们可以寻找并获得下一个元素。
但是，一旦返回的键大于 BBB，我们就需要停止。库提供了 raxCompare 函数，以避免你需要一遍又一
遍地编写相同的字符串比较函数:

    raxIterator iter;
    raxStart(&iter);
    raxSeek(&iter,">=",(unsigned char*)"AAA",3); // Seek the first element
    while(raxNext(&iter)) {
        if (raxCompare(&iter,">",(unsigned char*)"BBB",3)) break;
        printf("Current key: %.*s\n", (int)iter.key_len,(char*)iter.key);
    }
    raxStop(&iter);

The above code shows a complete range iterator just printing the keys
traversed by iterating.
上面的代码显示了一个完整的范围迭代器，只是打印迭代遍历的键。

The prototype of the `raxCompare` function is the following:
函数的原型如下:

    int raxCompare(raxIterator *iter, const char *op, unsigned char *key, size_t key_len);

The operators supported are `>`, `>=`, `<`, `<=`, `==`.
The function returns 1 if the current iterator key satisfies the operator
compared to the provided key, otherwise 0 is returned.

支持的操作符是 > >= < <= == 。如果与提供的键相比，当前迭代器键满足运算符的要求，则函数返回1，否则返回0。

## Checking for iterator EOF condition

Sometimes we want to know if the itereator is in EOF state before calling
raxNext() or raxPrev(). The iterator EOF condition happens when there are
no more elements to return via raxNext() or raxPrev() call, because either
raxSeek() failed to seek the requested element, or because EOF was reached
while navigating the tree with raxPrev() and raxNext() calls.

有时候，在调用 raxNext ()或 raxPrev ()之前，我们想知道迭代器是否处于 EOF 状态。当没有更多的元素通过
raxNext ()或 raxPrev ()调用返回时，由于 raxSeek ()未能寻找所请求的元素，或者由于在使用 raxPrev ()
和 raxNext ()调用导航树时达到了 EOF，就会出现迭代器 EOF 条件。

This condition can be tested with the following function that returns 1
if EOF was reached:

这个条件可以用下面的函数来测试，如果达到了 EOF，返回1:

    int raxEOF(raxIterator *it);

## Modifying the radix tree while iterating

In order to be efficient, the Rax iterator caches the exact node we are at,
so that at the next iteration step, it can start from where it left.
However an iterator has sufficient state in order to re-seek again
in case the cached node pointers are no longer valid. This problem happens
when we want to modify a radix tree during an iteration. A common pattern
is, for instance, deleting all the elements that match a given condition.

为了提高效率，Rax 迭代器缓存我们所处的精确节点，这样在下一个迭代步骤中，它可以从它离开的地方开始。
但是，迭代器具有足够的状态，以便在缓存的节点指针不再有效的情况下重新查找。当我们想在迭代期间修改基数树时，
就会出现这个问题。一个常见的模式是，例如，删除所有匹配给定条件的元素。

Fortunately there is a very simple way to do this, and the efficiency cost
is only paid as needed, that is, only when the tree is actually modified.
The solution consists of seeking the iterator again, with the current key,
once the tree is modified, like in the following example:

幸运的是，有一个非常简单的方法可以做到这一点，而且只有在需要的时候才付出效率成本，也就是说，
只有在树实际上被修改的时候才付出效率成本。解决方案包括在修改树之后，使用当前键再次寻找迭代器，如下例所示:

    while(raxNext(&iter,...)) {
        if (raxRemove(rax,...)) {
            raxSeek(&iter,">",iter.key,iter.key_size);
        }
    }

In the above case we are iterating with `raxNext`, so we are going towards
lexicographically greater elements. Every time we remove an element, what we
need to do is to seek it again using the current element and the `>` seek
operator: this way we'll move to the next element with a new state representing
the current radix tree (after the change).

在上面的例子中，我们使用 raxNext 进行迭代，因此我们将使用从字母顺序排列的更大元素。每次删除一个元素时，
我们需要做的是使用当前元素和 > seek 运算符再次查找该元素: 这样，我们将移动到下一个元素，并使用表示当前
基数树的新状态(在更改之后)。

The same idea can be used in different contexts, considering the following:

考虑到以下情况，同样的想法可以在不同的情况下使用:

* Iterators need to be sought again with `raxSeek` every time keys are added or removed while iterating.
* The current iterator key is always valid to access via `iter.key_size` and `iter.key`, even after it was deleted from the radix tree.

## Re-seeking iterators after EOF

After iteration reaches an EOF condition since there are no more elements
to return, because we reached one or the other end of the radix tree, the
EOF condition is permanent, and even iterating in the reverse direction will
not produce any result.

当迭代达到一个 EOF 条件，因为没有更多的元素返回，因为我们达到了基数树的一端或另一端，EOF
条件是永久的，即使在反向迭代也不会产生任何结果。

The simplest way to continue the iteration, starting again from the last
element returned by the iterator, is simply to seek itself:
从迭代器返回的最后一个元素开始，继续迭代的最简单的方法就是寻找它自己:

    raxSeek(&iter,iter.key,iter.key_len,"==");

So for example in order to write a command that prints all the elements
of a radix tree from the first to the last, and later again from the last
to the first, reusing the same iterator, it is possible to use the following
approach:

例如，为了编写一个命令，打印一个基数树的所有元素，从第一个到最后一个，然后再从最后一个到第一个，
重用相同的迭代器，可以使用以下方法:

    raxSeek(&iter,"^",NULL,0);
    while(raxNext(&iter,NULL,0,NULL))
        printf("%.*s\n", (int)iter.key_len, (char*)iter.key);

    raxSeek(&iter,"==",iter.key,iter.key_len);
    while(raxPrev(&iter,NULL,0,NULL))
        printf("%.*s\n", (int)iter.key_len, (char*)iter.key);

## Random element selection

To extract a fair element from a radix tree so that every element is returned
with the same probability is not possible if we require that:

从基数树中提取一个公平元素，以使每个元素都以相同的概率返回，如果我们要求:

1. The radix tree is not larger than expected (for example augmented with information that allows elements ranking).
2. We want the operation to be fast, at worst logarithmic (so things like reservoir sampling are out since it's O(N)).

However a random walk which is long enough, in trees that are more or less balanced, produces acceptable results, is fast, and eventually returns every possible element, even if not with the right probability.

然而，一个随机漫步是足够长的，在树上或多或少是平衡的，产生可接受的结果，是快速的，并最终返回每一个可能的元素，即使不是以正确的概率。

To perform a random walk, just seek an iterator anywhere and call the
following function:

要执行随机漫步，只需在任何地方寻找一个迭代器并调用以下函数:

    int raxRandomWalk(raxIterator *it, size_t steps);

If the number of steps is set to 0, the function will perform a number of
random walk steps between 1 and two times the logarithm in base two of the
number of elements inside the tree, which is often enough to get a decent
result. Otherwise, you may specify the exact number of steps to take.

如果步数设置为0，函数将执行一系列的随机游动步数，步数介于树中以2为底的元素数的对数的1到2倍之间，
这通常足以得到一个不错的结果。否则，您可以指定要采取的确切步骤数。

## Printing trees

For debugging purposes, or educational ones, it is possible to use the
following call in order to get an ASCII art representation of a radix tree
and the nodes it is composed of:

为了调试目的，或者为了教育目的，可以使用下面的调用来获得一个基数树和它所组成的节点的 ASCII 艺术表示:

    raxShow(mytree);

However note that this works well enough for trees with a few elements, but
becomes hard to read for very large trees.

但是请注意，这对于只有少量元素的树来说已经足够好了，但是对于非常大的树来说就很难阅读了。

The following is an example of the output raxShow() produces after adding
the specified keys and values:
下面是一个输出 raxShow ()在添加指定的键和值后生成的例子:

* alligator = (nil)
* alien = 0x1
* baloon = 0x2
* chromodynamic = 0x3
* romane = 0x4
* romanus = 0x5
* romulus = 0x6
* rubens = 0x7
* ruber = 0x8
* rubicon = 0x9
* rubicundus = 0xa
* all = 0xb
* rub = 0xc
* ba = 0xd

```
[abcr]
 `-(a) [l] -> [il]
               `-(i) "en" -> []=0x1
               `-(l) "igator"=0xb -> []=(nil)
 `-(b) [a] -> "loon"=0xd -> []=0x2
 `-(c) "hromodynamic" -> []=0x3
 `-(r) [ou]
        `-(o) [m] -> [au]
                      `-(a) [n] -> [eu]
                                    `-(e) []=0x4
                                    `-(u) [s] -> []=0x5
                      `-(u) "lus" -> []=0x6
        `-(u) [b] -> [ei]=0xc
                      `-(e) [nr]
                             `-(n) [s] -> []=0x7
                             `-(r) []=0x8
                      `-(i) [c] -> [ou]
                                    `-(o) [n] -> []=0x9
                                    `-(u) "ndus" -> []=0xa
```

# Running the Rax tests

To run the tests try:
测试
    $ make
    $ ./rax-test

To run the benchmark:
基准测试
    $ make
    $ ./rax-test --bench

To test Rax under OOM conditions:
OOM测试
    $ make
    $ ./rax-oom-test

The last one is very verbose currently.

In order to test with Valgrind, just run the tests using it, however
if you want accurate leaks detection, let Valgrind run the *whole* test,
since if you stop it earlier it will detect a lot of false positive memory
leaks. This is due to the fact that Rax put pointers at unaligned addresses
with `memcpy`, so it is not obvious where pointers are stored for Valgrind,
that will detect the leaks. However, at the end of the test, Valgrind will
detect that all the allocations were later freed, and will report that
there are no leaks.

为了在 Valgrind 进行测试，只需要使用它来运行测试，但是如果你想要准确地检测泄漏，就让 Valgrind
 运行整个测试，因为如果你提前停止它，它会检测到很多错误的正面内存泄漏。这是因为 Rax 把指针放在了
 未对齐的地址上，而 memcpy 是不对齐的，所以 Valgrind 的指针存储在哪里并不明显，这样就可以检测
 到泄漏。然而，在测试结束时，Valgrind 将检测到所有分配稍后被释放，并报告没有泄漏。

# Debugging Rax

While investigating problems in Rax it is possible to turn debugging messages
on by compiling with the macro `RAX_DEBUG_MSG` enabled. Note that it's a lot
of output, and may make running large tests too slow.

在 RAX 中调查问题时，可以在启用 rax_debug _ msg 宏的情况下通过编译打开调试消息。请注意，
输出量很大，可能会使运行大型测试过于缓慢。

In order to active debugging selectively in a dynamic way, it is possible to
use the function raxSetDebugMsg(0) or raxSetDebugMsg(1) to disable/enable
debugging.

为了以动态的方式有选择地激活调试，可以使用函数 raxSetDebugMsg (0)或 raxSetDebugMsg (1)
来禁用/启用调试。

A problem when debugging code doing complex memory operations like a radix
tree implemented the way Rax is implemented, is to understand where the bug
happens (for instance a memory corruption). For that goal it is possible to
use the function raxTouch() that will basically recursively access every
node in the radix tree, itearting every sub child. In combination with
tools like Valgrind, it is possible then to perform the following pattern
in order to narrow down the state causing a give bug:

当调试代码执行复杂的内存操作(如 Rax 实现方式的基数树)时，一个问题是要了解错误发生在哪里(例如内存损坏)。
为了实现这个目标，可以使用函数 raxTouch () ，它基本上可以递归地访问基数树中的每个节点，并为每个子节点
搜索。结合像 Valgrind 这样的工具，可以执行以下模式，以缩小导致 give bug 的状态:

1. The rax-test is executed using Valgrind, adding a printf() so that for
   the fuzz tester we see what iteration in the loop we are in.
2. After every modification of the radix tree made by the fuzz tester
   in rax-test.c, we add a call to raxTouch().
3. Now as soon as an operation will corrupt the tree, raxTouch() will
   detect it (via Valgrind) immediately. We can add more calls to narrow
   the state.
4. At this point a good idea is to enable Rax debugging messages immediately
   before the moment the tree is corrupted, to see what happens. This can
   be achieved by adding a few "if" statements inside the code, since we
   know the iteration that causes the corruption (because of step 1).

This method was used with success during rafactorings in order to debug the
introduced bugs.
