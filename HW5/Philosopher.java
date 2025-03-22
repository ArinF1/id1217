import java.io.*;
import java.net.*;
import java.util.Random;

public class Philosopher {
    private int id;
    private static final String SERVER_HOST = "localhost";
    private static final int SERVER_PORT = 12345;
    private Random rand;

    public Philosopher(int id) {
        this.id = id;
        rand = new Random();
    } 

    public void philosophyProcess() {
        try (Socket socket = new Socket(SERVER_HOST, SERVER_PORT);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true)) {

            // Loop indefinitely to think, request forks, eat, and release forks.
            while (true) {
                System.out.println("Philosopher " + id + " is thinking.");
                Thread.sleep(rand.nextInt(2000) + 1000);

                System.out.println("Philosopher " + id + " is hungry and waiting for forks.");
                out.println("REQUEST " + id);

                // Response will be "GRANTED" or continue to wait.
                String response = in.readLine();
                if ("GRANTED".equals(response)) {
                    System.out.println("Philosopher " + id + " is eating.");
                    Thread.sleep(rand.nextInt(2000) + 1000);

                    System.out.println("Philosopher " + id + " is done eating and puts down forks.");
                    out.println("DONE " + id);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Main method to start the philosopher process. The id is passed as an argument. 
    public static void main(String[] args) {
        if (args.length != 1) {
            System.out.println("Usage: java Philosopher <id>"); // id is the philosopher id
            System.exit(1); // exit if no id is provided
        }
        int id = Integer.parseInt(args[0]); // get philosopher id
        Philosopher philosopher = new Philosopher(id); // create a new philosopher object wit the id
        philosopher.philosophyProcess();
    }
}
