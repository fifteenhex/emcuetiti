package tests;


import org.fusesource.mqtt.client.BlockingConnection;
import org.fusesource.mqtt.client.Message;
import org.fusesource.mqtt.client.QoS;
import org.junit.Ignore;
import org.junit.Test;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;


public class TortureTest extends BaseMQTTTest {

    @Test
    public void subscribeAndUnsubscribeTorture() {
        BlockingConnection mqttConnection = mqttConnections[0];

        int i;
        for (i = 0; i < 100; i++) {
            subscribeToTopic(mqttConnection, TOPIC);
            if (!mqttConnection.isConnected())
                break;


            unsubFromTopic(mqttConnection, TOPIC);
            if (!mqttConnection.isConnected())
                break;

            try {
                Thread.sleep(10);
            } catch (InterruptedException ie) {
                throw new RuntimeException(ie);
            }
        }

        assert (i == 100);
    }

    @Test
    public void increasingPublishSize() {
        byte[] raw = new byte[64 * 1024];
        BlockingConnection sender = mqttConnections[0];
        BlockingConnection receiver = mqttConnections[1];

        subscribeToTopic(receiver, TOPIC);

        for (int size = 1; size < 64 * 1024; size *= 2) {
            byte[] block = Arrays.copyOf(raw, size);
            try {
                sender.publish(TOPIC, block, QoS.AT_MOST_ONCE, false);

                Message message = receiver.receive(10, TimeUnit.SECONDS);
                assert (message != null);

                byte[] outBlock = message.getPayload();
                assert (Arrays.equals(block, outBlock));

            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    @Test
    public void publishTorture() {
        String payload = "Hello";
        subscribeToTopic(mqttConnections[0], TOPIC);
        subscribeToTopic(mqttConnections[1], TOPIC2);
        try {
            for (int i = 0; i < 1024; i++) {
                exchange(mqttConnections[0], mqttConnections[1], TOPIC, payload, false);
                exchange(mqttConnections[1], mqttConnections[0], TOPIC2, payload, false);
            }
        } catch (Exception e) {
            throw new RuntimeException(e);
        } finally {
            unsubFromTopic(mqttConnections[0], TOPIC);
            unsubFromTopic(mqttConnections[1], TOPIC2);
        }
    }

}
