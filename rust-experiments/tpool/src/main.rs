use concurrent_queue::ConcurrentQueue;
use std::sync::Arc;
use std::thread;
use std::time::Duration;

pub trait Task: Send {
    fn run(&self);
}

struct ThreadPoolExecutor {
    num_threads: usize,
    queue_size: usize,
    queue: Arc<ConcurrentQueue<Box<dyn Task>>>,
}

impl ThreadPoolExecutor {
    fn new(num_threads: usize) -> Self {
        return ThreadPoolExecutor {
            num_threads: num_threads,
            queue_size: usize::MAX,
            queue: Arc::new(ConcurrentQueue::unbounded()),
        }
    }

    fn submit(&self, task: Box<dyn Task>) {
        self.queue.push(task);
    }

    fn start(&self) -> thread::JoinHandle<i32> {
        let q = Arc::clone(&self.queue);
        return thread::spawn(move || {
            let mut count: i32 = 0;
            for _ in 1..10 {
                println!("in thread");
                thread::sleep(Duration::from_millis(100));
                if let Ok(task) = q.pop() {
                    task.run();
                    count += 1;
                }
            }
            return count;
        });
    }
}

struct PrintTask {
    id: String,
}

impl Task for PrintTask {
    fn run(&self) {
        println!("printTask: {}", self.id);
    }
}

fn main() {
    let tp = ThreadPoolExecutor::new(10);
    let jh = tp.start();
    tp.submit(Box::new(PrintTask{ id: String::from("first") }));
    println!("Ran: {}", jh.join().unwrap());
}
