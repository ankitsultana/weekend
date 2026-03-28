mod seg_tree;

use seg_tree::SegTree;

enum ChildType {
    BothPresent,
    LeftPresent,
    RightPresent,
    NonePresent,
}

#[allow(dead_code)]
fn determine(st: &SegTree) -> ChildType {
    match (&st.left, &st.right) {
        (Some(_), Some(_)) => ChildType::BothPresent,
        (Some(_), None) => ChildType::LeftPresent,
        (None, Some(_)) => ChildType::RightPresent,
        (None, None) => ChildType::NonePresent,
    }
}

fn random_stuff() {
    let pr = ('a', 17);
    println!("{}", pr.0);
    println!("{}", pr.1);
}

fn gen_fn<L>(left: Option<L>) -> i32 {
    return match left {
        Some(blah) => 1,
        None => -1,
    }
}

fn gen_2(left: Option<(i32, i32)>) -> i32 {
    return match left {
        Some((x, y)) => x + y,
        None => -1,
    }
}

fn hello() {
    println!("Running hello");
    for c in "SurPRISE Inbound"
        .chars()
        .filter(|c| c.is_lowercase())
        .flat_map(|c| c.to_uppercase())
    {
        println!("{}", c);
    }
    println!("{}", gen_fn::<i32>(Some(3)));
    println!("{}", gen_2(Some((3, 4))));
    let x = (1, 23);
    let y2 = match x {
        (0, y) => y,
        (x, 0) => -x,
        _ => 0,
    };
    println!("{}", y2);
}

#[allow(dead_code)]
fn more_garbage() {
    println!("Hello, world!");
    let mut st = SegTree::new(10);
    println!("{}", st.value);
    st.left = Some(Box::new(SegTree {
        value: 1,
        left: Some(Box::new(SegTree::new(123))),
        right: None,
    }));
    st.right = Some(Box::new(SegTree {
        value: 2,
        left: None,
        right: None,
    }));
    let l: &SegTree = st.left.as_ref().unwrap();
    // let x: Option<&Box<SegTree>> = l.left.as_ref();
    // println!("{}", st.left.unwrap().value);
    let y: &Box<SegTree> = l.left.as_ref().unwrap();
    println!("{}", y.value);
    // println!("{}", st.right.unwrap().value);
    // println!("{}", determine(&st.left.unwrap()));
}

fn main() {
    hello();
    random_stuff();
}
