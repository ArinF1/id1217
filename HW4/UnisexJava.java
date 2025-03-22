import java.time.LocalTime;

/******************
 * * Monitor solution to unisex bathroom problem in java
 ******************/

class UnisexJava {
    // turn: 0 = noone turn, 1 = men turn, 2 = women turn.
    private int turn = 0;
    private int manInBathroom = 0;
    private int womanInBathroom = 0;
    private int manWaiting = 0;
    private int womanWaiting = 0;

    // java.time.LocalTime is for timestamp printing.
    private String timestamp() {
        return LocalTime.now().toString();
    }

    public synchronized void manEnter(int id) throws InterruptedException {
        // "waiting" for when blocking is necessary.
        if (womanInBathroom > 0 || (turn == 2 && womanWaiting > 0)) {
            System.out.println("Timestamp " + timestamp() + " - Man " + id + " waiting");
            manWaiting++;
            try {
                while (womanInBathroom > 0 || (turn == 2 && womanWaiting > 0)) {
                    wait();
                }
            } finally {
                manWaiting--;
            }
        } else {
            System.out.println("Timestamp " + timestamp() + " - Man " + id + " wants to enter");
        }
        // Now enter the bathroom.
        manInBathroom++;
        turn = 1; // Set to mens turn
        System.out.println("Timestamp " + timestamp() + " - Man " + id + " enters (menInside = " + manInBathroom + ").");
    }

    // called by a man thread to exit the bathroom.
    public synchronized void manExit(int id) {
        manInBathroom--;
        System.out.println("Timestamp " + timestamp() + " - Man " + id + " leaves (menInside = " + manInBathroom + ").");
        // Pass baton
        if (manInBathroom == 0 && womanWaiting > 0) {
            turn = 2;
        }
        notifyAll(); // Signal waiting threads
    }

    public synchronized void womanEnter(int id) throws InterruptedException {
        if (manInBathroom > 0 || (turn == 1 && manWaiting > 0)) {
            System.out.println("Timestamp " + timestamp() + " - Woman " + id + " waiting");
            womanWaiting++;
            try {
                while (manInBathroom > 0 || (turn == 1 && manWaiting > 0)) {
                    wait();
                }
            } finally {
                womanWaiting--;
            }
        } else {
            System.out.println("Timestamp " + timestamp() + " - Woman " + id + " wants to enter");
        }
        // Now enter the bathroom
        womanInBathroom++;
        turn = 2; // Set to womens turn
        System.out.println("Timestamp " + timestamp() + " - Woman " + id + " enters (womenInside = " + womanInBathroom + ").");
    }

    // Called by woman thread to exit the bathroom
    public synchronized void womanExit(int id) {
        womanInBathroom--;
        System.out.println("Timestamp " + timestamp() + " - Woman " + id + " leaves (womenInside = " + womanInBathroom + ").");
        // Pass baton if man is waiting and bathroom is empty
        if (womanInBathroom == 0 && manWaiting > 0) {
            turn = 1;
        }
        notifyAll(); // Signal waiting threads
    }
}

// Man thread
class Man implements Runnable {
    private UnisexJava bathroom;
    private int id;
    private int iterations;

    public Man(UnisexJava bathroom, int id, int iterations) {
        this.bathroom = bathroom;
        this.id = id;
        this.iterations = iterations;
    }

    public void run() {
        try {
            for (int i = 0; i < iterations; i++) {
                // Simulate work outside bathroom
                Thread.sleep((int) (Math.random() * 20000));
                bathroom.manEnter(id);
                // Simulate using the bathroom
                Thread.sleep((int) (Math.random() * 5000));
                bathroom.manExit(id);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}

// Woman thread
class Woman implements Runnable {
    private UnisexJava bathroom;
    private int id;
    private int iterations;

    public Woman(UnisexJava bathroom, int id, int iterations) {
        this.bathroom = bathroom;
        this.id = id;
        this.iterations = iterations;
    }

    public void run() {
        try {
            for (int i = 0; i < iterations; i++) {
                // Simulate working
                Thread.sleep((int) (Math.random() * 20000));
                bathroom.womanEnter(id);
                // Simulate using the bathroom
                Thread.sleep((int) (Math.random() * 5000));
                bathroom.womanExit(id);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}

// Main method
public class UnisexJavaTest {
    public static void main(String[] args) {
        final int numMen = 50;
        final int numWomen = 50;
        final int iterations = 1000000000; // Number of iteration for each thread

        UnisexJava bathroom = new UnisexJava();

        Thread[] menThreads = new Thread[numMen];
        Thread[] womenThreads = new Thread[numWomen];

        // Create and start man threads
        for (int i = 0; i < numMen; i++) {
            menThreads[i] = new Thread(new Man(bathroom, i, iterations));
            menThreads[i].start();
        }
        // Create and start woman threads
        for (int i = 0; i < numWomen; i++) {
            womenThreads[i] = new Thread(new Woman(bathroom, i, iterations));
            womenThreads[i].start();
        }

        // Join men threads
        for (int i = 0; i < numMen; i++) {
            try {
                menThreads[i].join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        for (int i = 0; i < numWomen; i++) {
            try {
                womenThreads[i].join();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }

        System.out.println("Simulation complete");
    }
}