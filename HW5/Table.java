import java.io.*;
import java.net.*;
import java.util.Arrays;

public class Table {
    private static final int PORT = 12345;
    // There are 5 forks (index --> 0-4). Philosopher i needs forks i and (i+1)%5. <-- wrap around in circular array.
    private boolean[] forks = new boolean[5]; // true if fork is free, false if fork is in use. <-- tracks fork availability
    final Object lock = new Object(); // lock object for synchronization, coordinates access to forks.

    // forks are free at start
    public Table() {
        Arrays.fill(forks, true);
    }

    // Server main method
    public static void main(String[] args) {
        Table server = new Table();
        server.start();
    }

    // server start method, This listens for philosopher connections.
    public void start() {
        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            System.out.println("Table server started on port " + PORT);
            while (true) {
                Socket clientSocket = serverSocket.accept();
                new Thread(new PhilosopherHandler(clientSocket, this)).start(); // Creates a new thread, runnable method.
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // pickups forks if available for a philosopher with the given id.
    public boolean pickUpForks(int id) {
        int fork1 = id;
        int fork2 = (id + 1) % 5; // circular array
        if (forks[fork1] && forks[fork2]) {
            forks[fork1] = false;
            forks[fork2] = false;
            System.out.println("Philosopher " + id + " is granted forks " + fork1 + " and " + fork2);
            return true;
        }
        return false;
    }

    // after philosopher is finished eating, this method is called to release the forks.
    public void putDownForks(int id) {
        int fork1 = id;
        int fork2 = (id + 1) % 5;
        forks[fork1] = true;
        forks[fork2] = true;
        System.out.println("Philosopher " + id + " released forks " + fork1 + " and " + fork2);

        // Notify waiting philosophers that forks are available.
        synchronized (lock) {
            lock.notifyAll();
        }
    }

    // This method implements the Runnable interface. this will handle each philosopher's requests.
    private static class PhilosopherHandler implements Runnable {
        private Socket socket;
        private Table table;
        private BufferedReader in;
        private PrintWriter out;

        // Constructor method, initializes the socket, table, and input/output streams.
        public PhilosopherHandler(Socket socket, Table table) {
            this.socket = socket;
            this.table = table;
            try {
                in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                out = new PrintWriter(socket.getOutputStream(), true);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

    // This method is called when a new thread is started.
        public void run() {
            try {
                String line;
                // Loop indefinitely to read requests from the philosopher.
                while ((line = in.readLine()) != null) {
                    if (line.startsWith("REQUEST")) {
                        int id = Integer.parseInt(line.split(" ")[1]); // get philosopher id
                        // If forks are not available, wait until notified.
                        synchronized (table.lock) {
                            while (!table.pickUpForks(id)) {
                                table.lock.wait();
                            }
                        }
                        out.println("GRANTED");  // If forks are available, grant access to the philosopher.
                    } else if (line.startsWith("DONE")) { // message for philosopher done eating
                        int id = Integer.parseInt(line.split(" ")[1]); // get philosopher id
                        table.putDownForks(id); // forks are released
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                try { socket.close(); } catch (IOException e) { } // close the socket
            }
        }
    }
}
