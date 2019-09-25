# coding=utf-8

'''
Python port of patricia trie
路由学习和路由查找
'''

from ipaddress import IPv4Network
from ipaddress import IPv6Network
from ipaddress import IPV4LENGTH
from ipaddress import IPV6LENGTH
from typing import Any
from sys import getsizeof
import unittest


class NetTrie(object):

    def __init__(self, ipv6=False):
        '''
        :param ipv6: 一个 NetTrie 只能是 IPv4 或 IPv6，不可以交叉
        '''
        self._is_v6 = ipv6
        self._root = None
        self._node_cnt = 0
        self._glue_cnt = 0
        self.max_prefixlen = IPV6LENGTH if ipv6 else IPV4LENGTH
        # MUST larger than IPv6 addr bits
        assert getsizeof(int) >= 16

    def __len__(self):
        return self._node_cnt

    def glue_cnt(self):
        return self._glue_cnt

    def best(self, addr: IPv4Network or IPv6Network):
        ''' search the best match
        :param addr:
        :return: (addr, bind_data)
        '''

        if addr.max_prefixlen != self.max_prefixlen:
            raise ValueError(f"fail match addr IP version, expect {self.max_prefixlen}")

        target = Node(addr, None, 0)
        next_node = self._root
        matches = []

        while next_node is not None and next_node.prefixlen < target.prefixlen:
            if next_node.addr is not None:
                matches.append(next_node)
            if target.bit_is_set(next_node.prefixlen + 1):
                next_node = next_node.right
            else:
                next_node = next_node.left

        if not matches:
            return None

        while len(matches) > 0:
            match = matches.pop()

            max_mask = (1 << self.max_prefixlen) - 1
            v1 = (1 << (self.max_prefixlen - match.prefixlen)) - 1
            v1 = v1 ^ max_mask
            mask = max_mask & v1
            match_val = int(match.addr.network_address)
            match_val = match_val & mask
            target_val = int(target.addr.network_address)
            target_val = target_val & mask
            if match_val == target_val:
                return match.addr, match.bind_data

        return None

    def exact(self, addr: IPv4Network or IPv6Network):
        '''
        查找正好等于 addr 的结点
        :param addr:
        :return: (addr, bind_data) or None
        '''

        if addr.max_prefixlen != self.max_prefixlen:
            raise ValueError(f"fail match addr IP version, expect {self.max_prefixlen}")
        target = Node(addr, None, 0)
        next_node = self._root

        while next_node is not None and next_node.prefixlen < target.prefixlen:
            if target.bit_is_set(next_node.prefixlen + 1):
                next_node = next_node.right
            else:
                next_node = next_node.left

        if next_node is None or next_node.addr is None or \
            next_node.prefixlen != target.prefixlen:
            return None

        if next_node.addr == addr:
            return next_node.addr, next_node.bind_data
        return None

    def insert(self, addr: IPv4Network or IPv6Network, data: Any):
        '''
        学习一个地址到路由表中
        只有在插入了后，才返回 (True, <Any>)
        如果插入一个已经插入过的相同网络地址，返回(False, <原有数据>)
        :param addr:
        :param data:
        :return: (bool: insert ok or not, (addr, bind_data) :already exist value)
        :raises
        '''
        if addr.max_prefixlen != self.max_prefixlen:
            raise ValueError(f"fail match addr IP version, expect {self.max_prefixlen}")

        new_node = Node(addr, data, 0)
        if self._root is None:
            self._root = new_node
            self._node_cnt += 1
            return True, None

        closet = self._find_tail(new_node)
        new_node_prefixlen = new_node.prefixlen

        final_same_bit = closet.fls(new_node) - 1
        final_same_bit = min(final_same_bit, self.max_prefixlen)

        fstnode = closet
        parent = fstnode.parent
        while parent is not None and parent.prefixlen > final_same_bit:
            fstnode = parent
            parent = parent.parent

        # find is self
        if final_same_bit == fstnode.prefixlen and \
            fstnode.prefixlen == new_node_prefixlen:
            return False, (fstnode.addr, fstnode.bind_data)

        if final_same_bit == fstnode.prefixlen:
            new_node.parent = fstnode

            if fstnode.prefixlen < self.max_prefixlen and \
                new_node.bit_is_set(fstnode.prefixlen + 1):

                if fstnode.right is not None:
                    raise ValueError("fstnode.right is not None")
                fstnode.right = new_node
            else:
                if fstnode.left is not None:
                    raise ValueError("fstnode.left is not None")
                fstnode.left = new_node
            self._node_cnt += 1
            return True, None

        if final_same_bit == new_node_prefixlen:
            n = closet
            if fstnode.addr is not None:
                n = fstnode
            if new_node_prefixlen < self.max_prefixlen and \
                n.bit_is_set(new_node_prefixlen + 1):
                new_node.right = fstnode
            else:
                new_node.left = fstnode
            self._insert_to_child(new_node, fstnode)
            self._node_cnt += 1
            return True, None

        glue = Node(None, None, final_same_bit)

        if final_same_bit < self.max_prefixlen and \
            new_node.bit_is_set(final_same_bit + 1):
            glue.left = fstnode
            glue.right = new_node
        else:
            glue.left = new_node
            glue.right = fstnode

        self._insert_to_child(glue, fstnode)
        new_node.parent = glue
        self._node_cnt += 1

        self._glue_cnt += 1
        return True, None

    def _find_tail(self, node):
        '''
        :param node:
        :return:
        '''
        prefixlen = node.prefixlen
        next = self._root

        while (next is not None) and \
            (next.prefixlen < prefixlen or next.addr is None):
            if next.prefixlen < self.max_prefixlen and node.bit_is_set(next.prefixlen + 1):
                if next.right is None:
                    break
                next = next.right
            else:
                if next.left is None:
                    break
                next = next.left
        return next

    def _insert_to_child(self, new_node, child):
        parent = child.parent
        new_node.parent = parent
        if parent is None:
            self._root = new_node
        elif parent.right == child:
            parent.right = new_node
        else:
            parent.left = new_node
        child.parent = new_node


class Node(object):
    __slots__ = ("addr", "parent", "left", "right", "bind_data",
                 "prefixlen")

    def __init__(self, addr: IPv4Network or IPv6Network, data: Any, prefixlen: int):
        '''
        :param addr: None or addr
        :param data:
        :param prefixlen:  0 if addr is not None
                           if addr is None, then use this value
        '''
        self.addr = addr
        self.parent = None
        self.left = None
        self.right = None
        self.bind_data = data
        self.prefixlen = prefixlen if addr is None else addr.prefixlen

    def __str__(self):
        if self.addr is None:
            return f"<glue>/{self.prefixlen}"
        return str(self.addr)

    def bit_is_set(self, bit: int) -> bool:
        ''' test network order bit set or not
        :param bit: bit > 0 && bit <= max_prefixlen
        '''
        int_val = int(self.addr.network_address)
        shift = self.addr.max_prefixlen
        return int_val & (1 << (shift - bit)) != 0

    def fls(self, other) -> int:
        '''
        find the last different bit (first bit of left order)
        :param other:
        :return: > 0
        '''
        chk_bits = min(self.addr.prefixlen, other.addr.prefixlen)
        left = int(self.addr.network_address)
        right = int(other.addr.network_address)
        xor_bit = left ^ right

        if xor_bit == 0:
            return chk_bits + 1
        r = self.addr.max_prefixlen - xor_bit.bit_length() + 1
        return min(r, chk_bits + 1)


class NodeTest(unittest.TestCase):
    def test_v6(self):
        addr = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/16"
        # 0xffff:0000:0000:0000:0000:0000:0000:0000
        obj = IPv6Network(addr, strict=False)
        self.assertEqual(int(obj.network_address), 340277174624079928635746076935438991360)
        self.assertEqual(str(hex(int(obj.network_address))), "0xffff0000000000000000000000000000")

    def test_bit_set(self):
        addr = "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/16"
        node = Node(IPv6Network(addr, strict=False), None, 0)
        self.assertEqual(node.bit_is_set(16), True)
        self.assertEqual(node.bit_is_set(1), True)
        self.assertEqual(node.bit_is_set(17), False)
        self.assertEqual(node.bit_is_set(node.addr.max_prefixlen - 1), False)

    def test_fls(self):
        node1 = Node(IPv4Network("127.0.0.1", strict=False), None, 0)
        node2 = Node(IPv4Network("1.0.0.2", strict=False), None, 0)
        self.assertEqual(node1.fls(node2), 2)


class NetTrieTest(unittest.TestCase):

    def test_v6_1(self):
        addr1 = IPv6Network("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/16", strict=False)
        addr2 = IPv6Network("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/32", strict=False)

        trie = NetTrie(ipv6=True)

        with self.assertRaises(ValueError):
            trie.insert(IPv4Network("1.1.1.1", strict=False), None)

        self.assertIsNone(trie.best(addr1))
        self.assertIsNone(trie.exact(addr1))

        self.assertEqual(trie.insert(addr1, None), (True, None))
        self.assertEqual(trie.best(addr2), (addr1, None))

    def test_v4_1(self):
        addr1 = IPv4Network("10.9.8.0/8", strict=False)
        addr2 = IPv4Network("10.9.8.0/9", strict=False)

        trie = NetTrie(ipv6=False)

        with self.assertRaises(ValueError):
            trie.insert(IPv6Network("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", strict=False), None)
        self.assertIsNone(trie.best(addr1))
        self.assertIsNone(trie.exact(addr1))

        self.assertEqual(trie.insert(addr1, None), (True, None))
        self.assertEqual(trie.best(addr2), (addr1, None))


if __name__ == '__main__':
    unittest.main()
