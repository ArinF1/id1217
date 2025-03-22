import java.io.*;
import java.net.*;
import java.util.*;
/*
 * Distributed Pairing Algorithm:
 *
 * The teacher listens for incoming student connections.
 * Each student sends their unique student id to the server when connecting.
 * When a student connects, if another student is already waiting, the server immediately
 * pairs the two by removing the waiting student from the list and notifying both students
 * of their partners
 * If no students are waiting the next student will be added to the wainting list
 * After all students have connected if the number of students is odd the remaining student
 * is paired with themselves.
 * This algorithm ensures that every student receives a partner in the order their requests are received.
 */

public class DistributedPeeringServer {

    //Waiting list for client
    private static List<ClientHandler> waitingClients = new ArrayList<>();
    // Total clients that have connected
    private static int totalClients = 0;
    // The expected number of clients
    private static int expectedClients = 0;
    
    //Pairs if there is another waiting client or add client to waiting
    public static synchronized void pairClient(ClientHandler client) {
        if (!waitingClients.isEmpty()) {
        

            ClientHandler partner = waitingClients.remove(0);
            client.setPartner(partner.getStudentId());
            partner.setPartner(client.getStudentId());
            System.out.println("Paired student " + client.getStudentId() + " with student " + partner.getStudentId());
        } else {
            waitingClients.add(client);
        }
    }
    
    public static void main(String[] args) {
        if (args.length < 2) {
            return;
        }
        
        int port = Integer.parseInt(args[0]);
        expectedClients = Integer.parseInt(args[1]);
        
        ServerSocket serverSocket = null;
        try {
            serverSocket = new ServerSocket(port);
            System.out.println("Server started on port " + port);
            
            // Keep track of all client threads so we can wait for them to finish.
            List<Thread> threads = new ArrayList<>();
            
            // Accept connections until we reach the expected number of students.
            while (totalClients < expectedClients) {
                Socket clientSocket = serverSocket.accept();
                totalClients++;
                ClientHandler handler = new ClientHandler(clientSocket);
                Thread t = new Thread(handler);
                t.start();
                threads.add(t);
            }
            
    
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
            }
            
           //Pair alone client if last one left after all have been accepted
            synchronized(DistributedPeeringServer.class) {
                for (ClientHandler client : waitingClients) {
                    client.setPartner(client.getStudentId());
                    System.out.println("Student " + client.getStudentId() + " is paired with themselves (odd number of students).");
                }
                waitingClients.clear();
            }
            
            // Wait for all client threads to finish before shutting down the server.
            for (Thread t : threads) {
                t.join();
            }
            
            System.out.println("All clients have been paired. Server shutting down.");
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if(serverSocket != null) {
                try { serverSocket.close(); } catch(Exception e) {}
            }
        }
    }
    

public static class ClientHandler implements Runnable {
        private Socket socket;
        private int studentId;
        private int partnerId = -1;  // -1 = no parter
        
        public ClientHandler(Socket socket) {
            this.socket = socket;
        }
        
        public int getStudentId() {
            return studentId;
        }
        
        //Notifies waiting thread when setting id
        public synchronized void setPartner(int partnerId) {
            this.partnerId = partnerId;
            notifyAll();
        }
        
        public void run() {
            try {
                DataInputStream in = new DataInputStream(socket.getInputStream());
                DataOutputStream out = new DataOutputStream(socket.getOutputStream());
                
                // Read the student id from the client.
                studentId = in.readInt();
                System.out.println("Received connection from student " + studentId);
                
                // Wrap the pairing and wait loop in one synchronized block.
                // so no notifications will be missed.
                synchronized(this) {
                    DistributedPeeringServer.pairClient(this);
                    while (partnerId == -1) {
                        wait();
                    }
                }
                
                // Send the partner ID back to the student.
                out.writeInt(partnerId);
                out.flush();
                
                socket.close();
            } catch(Exception e) {
                e.printStackTrace();
            }
        }
    }
}
