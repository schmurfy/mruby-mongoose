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
    p [:publish]
    # publish('/topic', "some data", id: 4, retain: true, qos: 1)
    publish('schmurfy/feeds/vcc', "10", retain: true)
    
    # subscribe('/test', qos: 0)
  end
  
  def puback(id)
    p [:puback, id]
  end
    
end

manager = Mongoose::Manager.new

manager.bind('tcp://127.0.0.1:1883', MQTTHandler)
  .set_protocol_mqtt()


manager.run(10000)
