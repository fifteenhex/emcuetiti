package tests;


import io.moquette.BrokerConstants;
import io.moquette.server.Server;
import org.fusesource.mqtt.client.BlockingConnection;
import org.fusesource.mqtt.client.MQTT;
import org.fusesource.mqtt.client.QoS;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.Properties;


public class Remote extends BaseMQTTTest {

    private static final String REMOTE_TOPIC = "/remote";
    private static final int REMOTE_PORT = 8992;
    private static final String REOTE_HOST = "127.0.0.1";
    private static final Server remoteBroker = new Server();
    private static BlockingConnection remoteClient;


    @BeforeClass
    public static void startRemoteBroker() {
        log("starting remote broker...");
        Properties props = new Properties();
        props.put(BrokerConstants.HOST_PROPERTY_NAME, REOTE_HOST);
        props.put(BrokerConstants.PORT_PROPERTY_NAME, Integer.toString(REMOTE_PORT));
        try {
            remoteBroker.startServer(props);
        } catch (IOException ioe) {
            throw new RuntimeException(ioe);
        }

        final MQTT mqttClient = new MQTT();
        try {
            mqttClient.setHost(REOTE_HOST, REMOTE_PORT);
            mqttClient.setConnectAttemptsMax(1);
            mqttClient.setReconnectAttemptsMax(0);
        } catch (URISyntaxException e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }


        /*remoteClient = mqttClient.blockingConnection();
        try {
            remoteClient.connect();
        } catch (Exception e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }*/


    }

    @AfterClass
    public static void stopRemoteBroker() {
        remoteBroker.stopServer();
    }


    @Test
    public void fromRemote() {
        String payload = "fromremote";
       /* try {
            remoteClient.publish(REMOTE_TOPIC, payload.getBytes(), QoS.AT_MOST_ONCE, false);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }*/

        try {
            Thread.sleep((60 * 1000) * 2);
        } catch (InterruptedException ie) {

        }

    }

    @Test
    public void toRemote() {

    }
}
