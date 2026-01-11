use std::fs::File;
use std::io;
use std::path::Path;
use std::cmp::Ordering;

use memmap2::Mmap;
use rand::Rng;

enum Event {
    KeyPress(char),
    SessionStart,
    SessionEnd,
}

struct Point {
    x: i32,
    y: i32
}

impl Point {
    fn dist(self) -> i32 {
        return self.x*self.x + self.y*self.y
    }
}

fn foo_bar(i: i32) -> i32 {
    return 3 * i
}

fn mmap_file<P: AsRef<Path>>(path: P) -> io::Result<Mmap> {
    let file = File::open(path)?;
    // SAFETY: Mapping the file for read-only access; file stays open for the map lifetime.
    unsafe { Mmap::map(&file) }
}

fn main() {
    hello();
    rngTest();
}

fn rngTest() {
    let x: Event = Event::KeyPress('s');
    match x {
        Event::KeyPress(c) => {
            println!("{c}");
        },
        _ => {
            println!("ignore");
        }
    }
    let secret_number = rand::thread_rng().gen_range(1..=100);
    println!("secret is: {secret_number}");

    loop {
        let mut guess = String::new();
        io::stdin()
            .read_line(&mut guess)
            .expect("Failed to read line");

        let guess: i32 = match guess.trim().parse() {
            Ok(num) => num,
            Err(_) => {
                println!("Yo you entered incorrect");
                break;
            },
        };
        println!("You guessed: {guess}");

        match guess.cmp(&secret_number) {
            Ordering::Less => println!("Too small!"),
            Ordering::Greater => println!("Too big!"),
            Ordering::Equal => {
                println!("You win!");
                break;
            }
        }
    }   
}

fn hello() {
    println!("{:?}", foo_bar(3));
    let x: (i32, &str) = (1, "hello");
    println!("{:?} {:?}", x.0, x.1);
    let _ = foo_bar(3);
    let y: i32 = { 42 };
    let mut y = 42;
    y = 3;
    let mut v1: Vec<i32> = Vec::new();
    println!("{:?}", v1.len());
    let natural_numbers = 1..;
    (3..6).contains(&4);

    let abc = String::from("apple");
    let x = &abc;
    // println!("{:?}", cnt(x));
    println!("{:?}", x.len());
    if let Ok(mmap) = mmap_file("Cargo.toml") {
        println!("Mapped file size: {}", mmap.len());
    }
}
