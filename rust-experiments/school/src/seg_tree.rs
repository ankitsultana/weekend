#[allow(dead_code)]
fn add2(x: i32, y: i32) -> i32 {
    return x + y;
}

pub struct SegTree {
    pub value: i32,
    pub left: Option<Box<SegTree>>,
    pub right: Option<Box<SegTree>>,
}

impl SegTree {
    pub fn new(value: i32) -> Self {
        return SegTree {
            value: value,
            left: None,
            right: None,
        };
    }
}
