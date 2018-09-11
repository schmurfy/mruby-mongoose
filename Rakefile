MRUBY_PATH = 
MRUBY_CONFIG = File.expand_path('../build_config.rb', __FILE__)

file :mruby do
  # sh "git clone --depth=1 git://github.com/mruby/mruby.git"
  sh "git clone -b 1.4.1 --depth=1 git://github.com/mruby/mruby.git"
end

desc "Update mongoose from github"
task :update_mongoose do
  Dir.chdir("./mongoose") do
    sh "rm mongoose*"
    sh "wget https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c"
    sh "wget https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h"
  end
end

desc "compile binary"
task :compile => :mruby do
  sh "cd mruby && MRUBY_CONFIG=#{MRUBY_CONFIG} rake all"
end

task :test => :compile do 
  sh "./mruby/build/host/bin/mruby examples/simple_webserver.rb"
end

task :debug => :compile do 
  sh "lldb ./mruby/build/host/bin/mruby examples/simple_webserver.rb"
end

# desc "test"
# task :test => :mruby do
#   sh "cd mruby && MRUBY_CONFIG=#{MRUBY_CONFIG} rake all test"
# end

desc "cleanup"
task :clean do
  sh "cd mruby && rake deep_clean"
end

task :default => :test
