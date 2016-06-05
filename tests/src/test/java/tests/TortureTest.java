package tests;


import org.fusesource.mqtt.client.BlockingConnection;
import org.junit.Ignore;
import org.junit.Test;

public class TortureTest extends BaseMQTTTest {

    @Ignore
    @Test
    public void subscribeAndUnsubscribeTorture() {
        BlockingConnection mqttConnection = mqttConnections[0];

        for (int i = 0; i < 100; i++) {
            subscribeToTopic(mqttConnection, TOPIC);
            unsubFromTopic(mqttConnection, TOPIC);
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
                throw new RuntimeException(ie);
            }
        }
    }

}
