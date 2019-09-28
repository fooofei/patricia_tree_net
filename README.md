# trie


[算法考试与路由有关系的]

「CQOI2016」路由表 
https://loj.ac/problem/2046#submit_code
https://www.cnblogs.com/outerform/p/5921840.html
http://oi.nks.edu.cn/jas/Problem/Details/3615

这题解法很水，就是循环遍历，没有看到有人使用基于树的查找

按照题目意思，增加计数的条件是从无到有，从短掩码匹配到长掩码匹配。


一个 IP 网络地址书写形式为 
1.1.1.0/25
其网络地址为 1.1.1.0 广播地址为 1.1.1.127

1.1.1.0/24 网络地址为  1.1.1.0 广播地址为 1.1.1.255

1.1.1.0/26 网络地址为 1.1.1.0  广播地址为 1.1.1.63


[PySubnetTree - A Python Module for CIDR Lookups]  半成品 lookup() 不存在时，返回 NULL， 非 Python 对象
https://github.com/zeek/pysubnettree

pytricia  这个还可以，能用 key 可以直接使用 python 官方模块 ipaddress
https://github.com/jsommers/pytricia


cidr-trie 功能不对 无法检测出子网
https://pypi.org/project/cidr-trie/#files

Maybe anothor from <https://github.com/brandt/network-patricia>

It's a bad code, not learn it.

I learn from <https://github.com/jsommers/pytricia/blob/master/patricia.c>


http://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=1494497

[没学到东西] https://stackoverflow.com/questions/24240902/what-is-the-most-efficient-way-to-match-the-ip-addresses-to-huge-route-entries

[那我的属于 Patricia 不压缩?] https://stackoverflow.com/questions/12709790/patricia-trie-for-fast-retrieval-of-ipv4-address-and-satellite-data

[Adaptive Radix Trees implemented in C] https://github.com/armon/libart

[Adaptive Radix Trees implemented in Go] https://github.com/plar/go-adaptive-radix-tree

[跟 Patricia 不是一回事][思路可以学习][路由之路由表查找算法概述] 
https://blog.csdn.net/dog250/article/details/6596046

```
https://github.com/jsommers/pytricia/blob/master/patricia.c
跟这个 Java 版本的最像
https://github.com/opendaylight/lispflowmapping/blob/c7a0bda035cba583b090a78614f7c4209f1229a8/mappingservice/inmemorydb/src/main/java/org/opendaylight/lispflowmapping/inmemorydb/radixtrie/RadixTrie.java
```

http://fxr.watson.org/fxr/source/include/net/xfrm.h?v=linux-2.6#L840


雷鹏
TerarkDB 数据库： 没有最快，只有更快
radix tree 是 patricia trie 的别名，有个 adaptive patricia trie ，核心思想就是针对不同的节点尺寸，使用不同的结点表达方式，控制内存的合理上界。
