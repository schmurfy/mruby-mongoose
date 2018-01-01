module DNSHandler
  def dns_message(msg)
    puts "Questions: "
    msg.questions.each do |q|
      p [q.name]
    end
    
    puts "\nAnswers: "
    msg.answers.each do |v|
      puts "[#{v.rtype}] #{v.name} => #{v.address}"
    end
  end
  
  def connected()
    send_dns_query("google.com", 1)
  end
end

manager = Mongoose::Manager.new

manager.connect('udp://8.8.8.8:53', DNSHandler)
  .set_protocol_dns()

manager.run(10000)
