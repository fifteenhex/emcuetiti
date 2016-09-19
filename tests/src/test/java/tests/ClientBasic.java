package tests;

import org.fusesource.mqtt.client.BlockingConnection;
import org.fusesource.mqtt.client.Message;
import org.fusesource.mqtt.client.QoS;
import org.fusesource.mqtt.client.Topic;
import org.junit.Ignore;
import org.junit.Test;

import java.util.concurrent.TimeUnit;

public class ClientBasic extends BaseMQTTTest {

    @Test
    public void subscribeAndUnsubscribe() {
        BlockingConnection mqttConnection = mqttConnections[0];
        subscribeToTopic(mqttConnection, TOPIC);
        unsubFromTopic(mqttConnection, TOPIC);
    }

    @Test
    public void subscribeAndUnsubscribeMultiple() {
        BlockingConnection mqttConnection = mqttConnections[0];
        String[] topics = new String[]{TOPIC, TOPIC2};
        subscribeToTopics(mqttConnection, topics);
        unsubFromTopics(mqttConnection, topics);
    }

    @Test
    public void subscribePublishUnsubscribeWildcard() {
        BlockingConnection listener = mqttConnections[0];
        BlockingConnection publisher = mqttConnections[1];
        subscribeToTopic(listener, TOPIC + "/#");
        try {
            exchange(listener, publisher, FULLSUBTOPIC, "hello", false);
        } catch (Exception e) {
            throw new RuntimeException(e);
        } finally {
            unsubFromTopic(listener, TOPIC + "/#");
        }
    }

    @Test
    public void publish() {
        String payload = "Hello";
        subscribeToTopic(mqttConnections[0], TOPIC);
        subscribeToTopic(mqttConnections[1], TOPIC2);
        try {
            exchange(mqttConnections[0], mqttConnections[1], TOPIC, payload, true);
            exchange(mqttConnections[1], mqttConnections[0], TOPIC2, payload, true);
        } catch (Exception e) {
            throw new RuntimeException(e);
        } finally {
            unsubFromTopic(mqttConnections[0], TOPIC);
            unsubFromTopic(mqttConnections[1], TOPIC2);
        }
    }

}
