
require 'rbconfig'
require 'mkmf'

MRuby::Gem::Specification.new('mruby-mongoose') do |spec|
  spec.license = 'MIT / GPLv2'
  spec.author  = 'Julien Ammous'
  spec.summary = 'Mongoose bindings (HTTP, WebSocket, DNS Client + Server, MQTT Client + Server, CoAP)'
  
  srcs =
    Dir.glob("#{dir}/mongoose/*.c") +
    Dir.glob("#{dir}/src/**/*.c") +
    Dir.glob("#{dir}/src/*.c")
  
  dir = File.dirname(__FILE__)
  
  spec.objs += srcs.map do |f|
    objfile(f.relative_path_from(@dir).to_s.pathmap("#{build_dir}/%X"))
  end
  
  spec.cc.include_paths << File.expand_path('../mongoose/', __FILE__)
  spec.cc.include_paths << File.expand_path('../mruby/include', __FILE__)
  # spec.linker.libraries << ''
  
  
  spec.export_include_paths << File.expand_path('../mongoose/', __FILE__)
  
  # spec.cc.defines += %w(
  #   -DMG_MALLOC=mrb_malloc
  #   -DMG_CALLOC=mrb_calloc
  # )
  
  spec.add_dependency 'mruby-error'
end
