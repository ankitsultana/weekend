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

    pub fn new_with_options(value: Box<V>, left: Option<Box<SegNode<V>>>, right: Option<Box<SegNode<V>>>) -> Self {
        return Self {
            value: value,
            left: left,
            right: right,
        };
    }

    pub fn new_with_inputs(value: Box<V>, left: Box<SegNode<V>>, right: Box<SegNode<V>>) -> Self {
        return Self {
            value: value,
            left: Some(left),
            right: Some(right),
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
