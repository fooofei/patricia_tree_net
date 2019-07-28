# trie


Maybe anothor from <https://github.com/brandt/network-patricia>

It's a bad code, not learn it.

I learn from <https://github.com/jsommers/pytricia/blob/master/patricia.c>


http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1494497
[没学到东西] https://stackoverflow.com/questions/24240902/what-is-the-most-efficient-way-to-match-the-ip-addresses-to-huge-route-entries
[那我的属于 Patricia 不压缩?] https://stackoverflow.com/questions/12709790/patricia-trie-for-fast-retrieval-of-ipv4-address-and-satellite-data
[Adaptive Radix Trees implemented in C] https://github.com/armon/libart
[Adaptive Radix Trees implemented in Go] https://github.com/plar/go-adaptive-radix-tree
[跟 Patricia 不是一回事][路由之路由表查找算法概述] https://blog.csdn.net/dog250/article/details/6596046
[PySubnetTree - A Python Module for CIDR Lookups]https://github.com/bro/pysubnettree

```
https://github.com/jsommers/pytricia/blob/master/patricia.c
跟这个 Java 版本的最像
https://github.com/opendaylight/lispflowmapping/blob/c7a0bda035cba583b090a78614f7c4209f1229a8/mappingservice/inmemorydb/src/main/java/org/opendaylight/lispflowmapping/inmemorydb/radixtrie/RadixTrie.java
```

http://fxr.watson.org/fxr/source/include/net/xfrm.h?v=linux-2.6#L840

```
static inline bool addr4_match(__be32 a1, __be32 a2, u8 prefixlen)
  841 {
  842         /* C99 6.5.7 (3): u32 << 32 is undefined behaviour */
  843         if (prefixlen == 0)
  844                 return true;
  845         return !((a1 ^ a2) & htonl(0xFFFFFFFFu << (32 - prefixlen)));
  846 }
```

雷鹏
TerarkDB 数据库： 没有最快，只有更快
radix tree 是 patricia trie 的别名，有个 adaptive patricia trie ，核心思想就是针对不同的节点尺寸，使用不同的结点表达方式，控制内存的合理上界。
