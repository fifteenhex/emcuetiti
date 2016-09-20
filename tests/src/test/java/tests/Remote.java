package tests;


import com.google.common.util.concurrent.SettableFuture;
import io.moquette.BrokerConstants;
import io.moquette.interception.InterceptHandler;
import io.moquette.interception.messages.*;
import io.moquette.server.Server;
import io.moquette.server.config.MemoryConfig;
import org.fusesource.mqtt.client.*;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.IOException;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;


public class Remote extends BaseMQTTTest {

    private static final String TOPICLOCALTOREMOTE = "remote/localtoremote";
    private static final String TOPICREMOTETOLOCAL = "remote/remotetolocal";
    private static final int REMOTE_PORT = 8992;
    private static final String REMOTE_HOST = "127.0.0.1";
    private static final Server remoteBroker = new Server();
    private static BlockingConnection remoteClient;

    private static final String REMOTECLIENTID = "remoteclient";
    private static SettableFuture<Boolean> emcuetitiConnected = SettableFuture.create();
    private static SettableFuture<Boolean> emcuetitiSubbed = SettableFuture.create();

    private static BlockingConnection localClient;

    private static InterceptHandler handler = new InterceptHandler() {
        @Override
        public void onConnect(InterceptConnectMessage msg) {
            log("Connected: " + msg.getClientID());
            if (msg.getClientID().equals(REMOTECLIENTID))
                emcuetitiConnected.set(true);
        }

        @Override
        public void onDisconnect(InterceptDisconnectMessage msg) {

        }

        @Override
        public void onPublish(InterceptPublishMessage msg) {

        }

        @Override
        public void onSubscribe(InterceptSubscribeMessage msg) {
            if (msg.getClientID().equals(REMOTECLIENTID))
                emcuetitiSubbed.set(true);
        }

        @Override
        public void onUnsubscribe(InterceptUnsubscribeMessage msg) {

        }
    };

    @BeforeClass
    public static void startClient() {
        localClient = createBlockingConnection(MQTT_HOSTNAME, MQTT_PORT, TOPICREMOTETOLOCAL);
    }

    @AfterClass
    public static void stopClient() {
        try {
            localClient.disconnect();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @BeforeClass
    public static void startRemoteBroker() {
        log("starting remote broker...");

        List<InterceptHandler> handlers = new ArrayList<>();
        handlers.add(handler);

        Properties props = new Properties();
        props.put(BrokerConstants.HOST_PROPERTY_NAME, REMOTE_HOST);
        props.put(BrokerConstants.PORT_PROPERTY_NAME, Integer.toString(REMOTE_PORT));

        try {
            remoteBroker.startServer(new MemoryConfig(props), handlers);
        } catch (IOException ioe) {
            throw new RuntimeException(ioe);
        }


        remoteClient = createBlockingConnection(REMOTE_HOST, REMOTE_PORT, TOPICLOCALTOREMOTE);
    }

    @AfterClass
    public static void stopRemoteBroker() {
        remoteBroker.stopServer();
    }

    private boolean getBoolFuture(SettableFuture<Boolean> future) {
        try {
            return future.get(1, TimeUnit.MINUTES);
        } catch (InterruptedException ie) {
            throw new RuntimeException(ie);
        } catch (TimeoutException te) {
            throw new RuntimeException(te);
        } catch (ExecutionException ee) {
            throw new RuntimeException(ee);
        }
    }


    @Test
    public void remoteToLocal() {
        getBoolFuture(emcuetitiConnected);
        getBoolFuture(emcuetitiSubbed);

        String payload = "fromremote";
        try {
            remoteClient.publish(TOPICREMOTETOLOCAL, payload.getBytes(), QoS.AT_MOST_ONCE, false);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        try {
            localClient.receive(30, TimeUnit.SECONDS);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        try {
            Thread.sleep((60 * 1000));
        } catch (InterruptedException ie) {

        }
    }


    @Test
    public void localToRemote() {
        getBoolFuture(emcuetitiConnected);
        getBoolFuture(emcuetitiSubbed);

        /*String payload = "toremote";
        byte[] payloadbytes = payload.getBytes();
        try {
            localClient.publish(TOPICLOCALTOREMOTE, payloadbytes, QoS.AT_MOST_ONCE, false);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        try {
            Message message = remoteClient.receive(30, TimeUnit.SECONDS);
            assert message != null;
            String payloadIn = new String(message.getPayload(), "UTF-8");
            assert payloadIn.equals(payload) : "wanted [" + payload + "] got [" + payloadIn + "]";
        } catch (Exception e) {
            throw new RuntimeException(e);
        }*/

        try {
            Thread.sleep((60 * 1000));
        } catch (InterruptedException ie) {

        }
    }


}
