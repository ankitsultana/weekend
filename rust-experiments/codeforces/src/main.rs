use std::{
    collections::VecDeque,
    fmt,
    io::{self, BufRead},
    str::FromStr,
};

/////////////////////////
// Copied Scanner code //
/////////////////////////

struct Scanner {
    tokens: VecDeque<String>,
}

impl Scanner {
    pub fn new() -> Self {
        let stdin = io::stdin();
        let mut tokens = VecDeque::new();
        for line in stdin.lock().lines() {
            for token in line.unwrap().split_ascii_whitespace() {
                tokens.push_back(token.to_owned());
            }
        }
        Self { tokens }
    }

    pub fn next<T: FromStr>(&mut self) -> T
    where
        <T as FromStr>::Err: fmt::Debug,
    {
        T::from_str(&self.tokens.pop_front().unwrap()).unwrap()
    }
}

/////////////////////////
// Custom Segtree code //
/////////////////////////

pub struct SegNode<V> {
    pub value: Box<V>,
    pub left: Option<Box<SegNode<V>>>,
    pub right: Option<Box<SegNode<V>>>,
}

impl<V: std::fmt::Display> SegNode<V> {
    pub fn new(value: Box<V>) -> Self {
        return Self {
            value: value,
            left: None,
            right: None,
        };
    }

    pub fn print(&self) {
        println!("SegNode({})", self.value);
    }

    pub fn print_r(&self) {
        self.print();
        if let Some(ref l) = self.left {
            l.print_r();
        }
        if let Some(ref r) = self.right {
            r.print_r();
        }
    }
}

struct SegTree<V: std::fmt::Display> {
    pub root: SegNode<V>,
    pub range: i32,
}

impl SegTree<i32> {
    pub fn new(range: i32, init_val: Box<i32>) -> Self {
        return Self {
            root: SegNode::<i32>::new(init_val),
            range: range,
        }
    }

    pub fn update(&mut self, l: i32, r: i32, x: i32) {
        Self::update_internal(&mut self.root, 1, self.range, l, r, x);
    }

    pub fn query(&self) -> Vec<i32> {
        let mut v: Vec<i32> = vec![0; (self.range + 1).try_into().unwrap()];
        let mut stk: Vec<i32> = Vec::new();
        Self::query_internal(&self.root, 1, self.range.try_into().unwrap(), &mut v, &mut stk);
        return v;
    }

    fn update_internal(node: &mut SegNode<i32>, l: i32, r: i32, target_l: i32, target_r: i32, x: i32) {
        if *node.value > 0 {
            return;
        } else if l >= target_l && r <= target_r {
            node.value = Box::new(x);
            return;
        } else if l > target_r || r < target_l {
            return;
        }
        let mid: i32 = (l + r) / 2;
        if node.left.is_none() {
            node.left = Some(Box::new(SegNode::<i32>::new(Box::new(0))));
        }
        if node.right.is_none() {
            node.right = Some(Box::new(SegNode::<i32>::new(Box::new(0))));
        }
        Self::update_internal(node.left.as_mut().unwrap(), l, mid, target_l, target_r, x);
        Self::update_internal(node.right.as_mut().unwrap(), mid + 1, r, target_l, target_r, x);
    }

    fn query_internal(node: &SegNode<i32>, l: usize, r: usize, result: &mut Vec<i32>, stk: &mut Vec<i32>) {
        if l == r {
            if *node.value > 0 && *node.value != l.try_into().unwrap() {
                result[l] = *node.value;
            } else {
                let l_int: i32 = l.try_into().unwrap();
                for i in stk.iter().rev() {
                    if *i != l_int {
                        result[l] = *i;
                        break;
                    }
                }
            };
            return;
        }
        stk.push(*node.value);
        let mid: usize = (l + r) / 2;
        if let Some(node_l) = node.left.as_ref() {
            Self::query_internal(node_l, l, mid, result, stk);
        }
        if let Some(node_r) = node.right.as_ref() {
            Self::query_internal(node_r, mid + 1, r, result, stk);
        }
        stk.pop();
    }
}

fn main() {
    let mut sc = Scanner::new();
    let n: i32 = sc.next();
    let m: i32 = sc.next();
    let mut tree = SegTree::<i32>::new(n, Box::new(0));
    for _ in 0..m {
        let l: i32 = sc.next();
        let r: i32 = sc.next();
        let x: i32 = sc.next();
        tree.update(l, r, x);
    }
    let v: Vec<i32> = tree.query();
    for i in 1..(n+1) {
        print!("{} ", v[i as usize]);
    }
}
