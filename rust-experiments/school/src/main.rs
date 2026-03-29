mod seg_tree;

use crate::seg_tree::SegNode;

fn hello() {
    let left = SegNode::<i32>::new(Box::new(1));
    let right = SegNode::<i32>::new_with_options(Box::new(2), None, None);
    let s = SegNode::<i32>::new_with_inputs(Box::new(32), Box::new(left), Box::new(right));
    s.print_r();
}

fn main() {
    hello();
}
