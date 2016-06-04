package tests;

import java.io.IOException;
import java.net.URISyntaxException;

import org.fusesource.mqtt.client.BlockingConnection;
import org.fusesource.mqtt.client.MQTT;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;

public class BaseMQTTTest {

	private static final boolean STARTBROKER = false;
	private static final String MQTT_HOSTNAME = "localhost";
	private static final int MQTT_PORT = 8991;
	private static final int MQTT_CLIENTS = 2;

	private static Process brokerProcess;
	protected static BlockingConnection[] mqttConnections = new BlockingConnection[MQTT_CLIENTS];

	public static void log(String message) {
		System.out.println(message);
	}

	@BeforeClass
	public static void startBrokerAndCreateClients() {
		if (STARTBROKER) {
			log("Starting broker...");

			ProcessBuilder brokerProcessBuilder = new ProcessBuilder("../emcuetiti_linux");
			brokerProcessBuilder.inheritIO();
			try {
				brokerProcess = brokerProcessBuilder.start();
			} catch (IOException e) {
				e.printStackTrace();
				throw new RuntimeException(e);
			}
		}

		log("Creating clients...");

		final MQTT mqttClient = new MQTT();
		try {
			mqttClient.setHost(MQTT_HOSTNAME, MQTT_PORT);
			mqttClient.setConnectAttemptsMax(1);
		} catch (URISyntaxException e) {
			e.printStackTrace();
			throw new RuntimeException(e);
		}

		for (int c = 0; c < MQTT_CLIENTS; c++) {
			final BlockingConnection mqttConnection = mqttClient.blockingConnection();
			try {
				mqttConnection.connect();
			} catch (Exception e) {
				e.printStackTrace();
				throw new RuntimeException(e);
			}

			mqttConnections[c] = mqttConnection;
		}
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

}
