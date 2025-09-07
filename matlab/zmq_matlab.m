javaaddpath('/home/sceptik/java_libs/jeromq-0.5.3.jar');

import org.zeromq.*;

port = 4000;

%create socket
context = ZMQ.context(1);
socket = context.socket(ZMQ.REQ);
socket.connect(sprintf('tcp://*:%d', port));


%creat exit button
f = figure('Name', 'ZeroMQ Control', 'NumberTitle', 'off', ...
           'Position', [300, 300, 200, 100]);
uicontrol('Parent', f, ...
          'Style', 'pushbutton', ...
          'String', 'EXIT', ...
          'Position', [50, 30, 100, 40], ...
          'Callback', @(src, event) toggleExit());

global flag;
flag = true;

%for REQ pattern
socket.send("Hello", ZMQ.DONTWAIT);


while flag
    drawnow;
    pause(0.05);

    data = socket.recv(ZMQ.DONTWAIT);
    if ~isempty(data)   % data always empty
        disp(length(data));
        socket.send(data, ZMQ.DONTWAIT);
    end

end

disp("Start clear resource");
socket.close();
disp("End clear resource");

function toggleExit()
    global flag;
    flag = ~flag;
end