import java.io.*;
import java.net.*;

public class DistributedPeering {
    public static void main(String[] args) {
        if (args.length < 3) {
            return;
        }
        
        String serverHost = args[0];
        int serverPort = Integer.parseInt(args[1]);
        int studentId = Integer.parseInt(args[2]);
        
        try {
            Socket socket = new Socket(serverHost, serverPort);
            DataOutputStream out = new DataOutputStream(socket.getOutputStream());
            DataInputStream in = new DataInputStream(socket.getInputStream());
            
            // Send the student id to the server.
            out.writeInt(studentId);
            out.flush();
            
            // Wait for the server to send the partner id.
            int partnerId = in.readInt();
            
            if (partnerId == studentId) {
                System.out.println("Student " + studentId + " is paired with themselves.");
            } else {
                System.out.println("Student " + studentId + " is paired with student " + partnerId);
            }
            
            socket.close();
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
}
