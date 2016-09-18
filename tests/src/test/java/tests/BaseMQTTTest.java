package tests;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.Properties;
import java.util.concurrent.TimeUnit;

import io.moquette.BrokerConstants;
import io.moquette.server.Server;
import io.moquette.server.config.MemoryConfig;
import org.fusesource.mqtt.client.*;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;

public class BaseMQTTTest {

    private static final boolean STARTBROKER = false;
    protected static final String MQTT_HOSTNAME = "localhost";
    protected static final int MQTT_PORT = 8991;
    private static final int MQTT_CLIENTS = 2;


    protected static final String TOPIC = "topic1";
    protected static final String SUBTOPIC = "subtopic1";
    protected static final String TOPIC2 = "topic2";
    protected static final String FULLSUBTOPIC = TOPIC + "/" + SUBTOPIC;

    private static Process brokerProcess;
    protected static BlockingConnection[] mqttConnections = new BlockingConnection[MQTT_CLIENTS];

    public static void log(String message) {
        System.out.println(message);
    }

    public static BlockingConnection createBlockingConnection(String host, int port) {
        return createBlockingConnection(host, port, null);
    }

    public static BlockingConnection createBlockingConnection(String host, int port, String topic) {
        final MQTT mqttClient = new MQTT();
        try {
            mqttClient.setHost(host, port);
            mqttClient.setConnectAttemptsMax(1);
            mqttClient.setReconnectAttemptsMax(0);
            mqttClient.setCleanSession(true);
        } catch (URISyntaxException e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }

        final BlockingConnection mqttConnection = mqttClient.blockingConnection();
        try {
            mqttConnection.connect();

        } catch (Exception e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }

        if (topic != null) {
            Topic[] topics = new Topic[]{new Topic(topic, QoS.AT_MOST_ONCE)};
            try {
                mqttConnection.subscribe(topics);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }

        return mqttConnection;
    }

    @BeforeClass
    public static void startBrokerAndCreateClients() {
        if (STARTBROKER) {
            ProcessBuilder brokerProcessBuilder = new ProcessBuilder("../build/emcuetiti_linux",
                    "-p", Integer.toString(MQTT_PORT),
                    "-t", TOPIC,
                    "-t", TOPIC2,
                    "-t", FULLSUBTOPIC);
            brokerProcessBuilder.inheritIO();

            log("Starting broker...");

            try {
                brokerProcess = brokerProcessBuilder.start();
            } catch (IOException e) {
                e.printStackTrace();
                throw new RuntimeException(e);
            }
        }

        log("Creating clients...");


        for (int c = 0; c < MQTT_CLIENTS; c++)
            mqttConnections[c] = createBlockingConnection(MQTT_HOSTNAME, MQTT_PORT);

    }

    @AfterClass
    public static void disconnectClientsAndStopBroker() {
        log("Disconnecting clients...");

        for (int c = 0; c < MQTT_CLIENTS; c++) {
            if (mqttConnections[c] != null) {
                final BlockingConnection mqttConnection = mqttConnections[c];
                mqttConnections[c] = null;

                try {
                    if (mqttConnection.isConnected())
                        mqttConnection.disconnect();
                } catch (Exception e) {
                    e.printStackTrace();
                    throw new RuntimeException(e);
                }
            }
        }

        if (STARTBROKER) {
            log("Shutting down broker...");

            brokerProcess.destroy();
            brokerProcess = null;
        }
    }

    @Before
    public void checkConnected() {
        for (int c = 0; c < mqttConnections.length; c++) {
            assert (mqttConnections[c].isConnected());
        }
    }

    protected void subscribeToTopic(final BlockingConnection mqttConnection, final String topic) {
        try {
            Topic[] topics = new Topic[]{new Topic(topic, QoS.AT_LEAST_ONCE)};
            mqttConnection.subscribe(topics);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    protected void unsubFromTopic(final BlockingConnection mqttConnection, final String topic) {
        try {
            mqttConnection.unsubscribe(new String[]{topic});
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    protected void subscribeToTopics(final BlockingConnection mqttConnection, final String[] topic) {
        try {
            Topic[] topics = new Topic[topic.length];
            for (int i = 0; i < topics.length; i++) {
                Topic t = new Topic(topic[i], QoS.AT_LEAST_ONCE);
                topics[i] = t;
            }
            mqttConnection.subscribe(topics);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    protected void unsubFromTopics(final BlockingConnection mqttConnection, final String[] topics) {
        try {
            mqttConnection.unsubscribe(topics);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    protected void exchange(BlockingConnection listener, BlockingConnection publisher, String topic, String payload, boolean checkPublisher) throws Exception {

        publisher.publish(topic, payload.getBytes(), QoS.AT_MOST_ONCE, false);

        // check that the client subbed to the topic gets the publish and it matches
        Message receivedPublish = listener.receive(10, TimeUnit.SECONDS);
        assert (receivedPublish != null) : "should have received a publish, didn't get one";
        assert (receivedPublish.getTopic().equals(topic)) : "wanted topic " + topic + " but got " + receivedPublish.getTopic();
        String payloadIn = new String(receivedPublish.getPayload(), "UTF-8");
        assert (payloadIn.equals(payload));


        if (checkPublisher) {
            // check that the publishing client didn't get the publish
            Message shouldNotReceivePublish = publisher.receive(10, TimeUnit.SECONDS);
            assert (shouldNotReceivePublish == null) : "shouldn't have received a publish but got one";
        }
    }

}
