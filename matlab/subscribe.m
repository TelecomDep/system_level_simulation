
clear java;
javaaddpath('/home/andrey/jeromq/target/jeromq-0.6.0.jar')

import org.zeromq.ZMQ.*;
import org.zeromq.*;

port_api = 2111;
context = ZMQ.context(1);
socket_api_proxy = context.socket(ZMQ.REP);
socket_api_proxy.bind(sprintf('tcp://*:%d', port_api));

fprintf("Start")
figure(1);
global pauseFlag;
pauseFlag = false;
uicontrol('Style', 'pushbutton', 'String', 'Pause/Resume', ...
              'Position', [20, 20, 100, 30], ...
              'Callback', @(src, event) togglePause());
while true
    
    if ~pauseFlag
        msg = socket_api_proxy.recv();
        if ~isempty(msg)
            fprintf('received message [%d]\n', length(msg));
            if(length(msg) > 1000)
                process_data(msg);
            end
            socket_api_proxy.send("OK");
        end
    else
        pause(0.1);
    end
end
function togglePause()
    global pauseFlag;
    pauseFlag = ~pauseFlag;
end
function process_data(data_raw)
    fs = 23040000;
    fprintf("size data: %d\n", length(data_raw));
    data_slice = data_raw;
    floatArray = typecast(uint8(data_slice), 'single');
    complexArray = complex(floatArray(1:2:end), floatArray(2:2:end));
    data_complex = complexArray(1:128*180);
    fprintf("size complex data: %d\n", length(data_complex));
    cla;
    window = 128;    
    noverlap = 0; 
    nfft = 128;      
    if any(isnan(data_complex))
        data_complex(isnan(data_complex)) = 0;
    end
    subplot(2, 2, 1);
    x_t = 1:length(data_complex);
    plot(x_t, data_complex);
    title('Данные в временной области');
    xlabel('Отсчеты');
    ylabel('Амплитуда');
    subplot(2, 2, 2);
    spectrogram(data_complex, window, noverlap, nfft, fs, 'yaxis');
    title('Спектрограмма переданных данных');
    xlabel('Время (сек)');
    ylabel('Частота (Гц)');
    colorbar;
    grid on;
    drawnow;
end


























