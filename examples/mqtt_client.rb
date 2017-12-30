module MQTTHandler
  
  def connection_failed(reason)
    p [:failed, reason]
  end
  
  def published(topic, data)
    p [:published, topic, data]
  end
  
  def connected
    set_timer(200)
    p [:connected]
  end
  
  def timer
    # publish('/topic', "some data", id: 4, retain: true, qos: 1)
    publish('schmurfy/feeds/vcc', "10", retain: true)
    
    # subscribe('/test', qos: 0)
  end
  
  def puback(id)
    p [:puback, id]
  end
    
end

manager = Mongoose::Manager.new

# manager.connect('tcp://127.0.0.1:1883', MQTTHandler)
#   .authenticate_with('user', 'pass')
#   .set_protocol_mqtt()

manager.connect('tcp://io.adafruit.com:1883', MQTTHandler)
  .authenticate_with('schmurfy', '554d36681eeda7fcb08f35705aa4e12d60741952')
  .set_protocol_mqtt()


manager.run(10000)
