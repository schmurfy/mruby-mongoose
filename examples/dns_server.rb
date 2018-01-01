module DNSHandler
  
  def dns_message(msg)
    reply = Mongoose::DNS::Reply.new(msg)
    
    msg.questions.each do |q|
      if q.rtype == :A
        reply.add_record(q, rdata_ipv4: "10.2.36.4", ttl: 24)
      end
    end
    
    
    send_dns_reply(reply)
  end
end

manager = Mongoose::Manager.new

manager.bind('udp://127.0.0.1:5533', DNSHandler)
  .set_protocol_dns()

manager.run(10000)
