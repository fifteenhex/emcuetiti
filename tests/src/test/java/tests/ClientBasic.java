package tests;

import org.fusesource.mqtt.client.BlockingConnection;
import org.fusesource.mqtt.client.Message;
import org.fusesource.mqtt.client.QoS;
import org.fusesource.mqtt.client.Topic;
import org.junit.Test;

import java.util.concurrent.TimeUnit;

public class ClientBasic extends BaseMQTTTest {

    private static final String TOPIC = "/topic1";

    private void subscribeToTopic(final BlockingConnection mqttConnection, final String topic) {
        try {
            Topic[] topics = new Topic[]{new Topic(topic, QoS.AT_LEAST_ONCE)};
            mqttConnection.subscribe(topics);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private void unsubFromTopic(final BlockingConnection mqttConnection, final String topic) {
        try {
            mqttConnection.unsubscribe(new String[]{topic});
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Test
    public void subscribeAndUnsubscribe() {
        BlockingConnection mqttConnection = mqttConnections[0];
        subscribeToTopic(mqttConnection, TOPIC);
        unsubFromTopic(mqttConnection, TOPIC);
    }

    @Test
    public void publish() {
        BlockingConnection listener = mqttConnections[0];
        BlockingConnection publisher = mqttConnections[1];

        subscribeToTopic(listener, TOPIC);

        String payloadOut = "Hello";

        try {
            publisher.publish(TOPIC, payloadOut.getBytes(), QoS.AT_MOST_ONCE, false);

            Message receivedPublish = listener.receive(10, TimeUnit.SECONDS);
            assert (receivedPublish != null);

            String payloadIn = new String(receivedPublish.getPayload(), "UTF-8");

            assert (payloadIn.equals(payloadOut));

        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        unsubFromTopic(listener, TOPIC);
    }

}
