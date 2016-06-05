package tests;


import org.fusesource.mqtt.client.BlockingConnection;
import org.junit.Ignore;
import org.junit.Test;

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
                Thread.sleep(1000);
            } catch (InterruptedException ie) {
                throw new RuntimeException(ie);
            }
        }

        assert (i == 100);
    }

}
