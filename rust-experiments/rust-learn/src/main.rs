use std::cmp::Ordering;
use std::io;

use rand::Rng;

enum Event {
    KeyPress(char),
    SessionStart,
    SessionEnd,
}

fn main() {
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
