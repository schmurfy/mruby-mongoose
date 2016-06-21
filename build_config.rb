MRuby::Build.new do |conf|
  toolchain :gcc
  enable_debug
  conf.enable_test
  
  conf.gem(core: 'mruby-bin-mruby')
  conf.gem(core: 'mruby-print')
  conf.gem File.expand_path(File.dirname(__FILE__)) do
    
    # for HTTPS
    cc.include_paths << '/usr/local/Cellar/openssl/1.0.2h_1/include'
    linker.libraries += %w(crypto ssl)
    
    # add ipv6 support
    # cc.defines << "MG_ENABLE_IPV6"
    
    # use read/write instead of recv/send, allow using
    # serial devices or file descriptors
    cc.defines << "MG_USE_READ_WRITE"
    
    # enable optional features
    # cc.defines << "MG_ENABLE_MQTT_BROKER"
    cc.defines << "MG_ENABLE_DNS_SERVER"
    cc.defines << "MG_ENABLE_SSL"
    # cc.defines << "MG_ENABLE_COAP"
    # cc.defines << "MG_ENABLE_GETADDRINFO"
    
    # disable default features
    # cc.defines << "MG_DISABLE_HTTP_WEBSOCKET"
    # cc.defines << "MG_DISABLE_SHA1" # used by websocket
    # cc.defines << "MG_DISABLE_HTTP_DIGEST_AUTH"
    # cc.defines << "MG_DISABLE_MD5" # used by HTTP
    # cc.defines << "MG_DISABLE_MQTT"
    # cc.defines << "MG_DISABLE_JSON_RPC"
    # cc.defines << "MG_DISABLE_HTTP_KEEP_ALIVE"
    
    cc.defines << "MG_DISABLE_SOCKETPAIR" # mg_broadcast
    cc.defines << "MG_DISABLE_CGI" # requires MG_DISABLE_SOCKETPAIR
  end
end
