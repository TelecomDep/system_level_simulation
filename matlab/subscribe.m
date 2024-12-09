
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
                % process_data1(msg);
            end
            socket_api_proxy.send(msg);
        end
    else
        pause(0.1);
    end
end
function togglePause()
    global pauseFlag;
    pauseFlag = ~pauseFlag;
end
function process_data1(data_raw)
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
    
    % Расчет потерь сигнала по модели COST 231 Hata
    fc = 900; % Частота в МГц
    hte = 50; % Высота передающей антенны в метрах
    hre = 1.5; % Высота приемной антенны в метрах
    d = 5; % Расстояние между передатчиком и приемником в километрах
    Cm = 0; % Поправочный коэффициент для средних городов и пригородов
    
    % Расчет поправочного коэффициента для высоты приемной антенны
    a_hre = (1.1 * log10(fc) - 0.7) * hre - (1.56 * log10(fc) - 0.8);
    
    % Расчет потерь сигнала
    L = 46.3 + 33.9 * log10(fc) - 13.82 * log10(hte) - a_hre + (44.9 - 6.55 * log10(hte)) * log10(d) + Cm;
    
    % Вывод результата
    % transmission_channel_model()
    subplot(2, 2, 3);
    plot(x_t, data_complex);
    text(0.5, 0.5, sprintf('Потери сигнала: %.2f дБ', L), 'HorizontalAlignment', 'center', 'FontSize', 14);
    title('Потери сигнала по модели COST 231 Hata');
    axis off;
    
    drawnow;
end


function out_data = transmission_channel_model(data, c, Nb, f0, Ts, D1, Dn, N0)
    D = randi([D1, Dn], 1, Nb);
    PRINT_DEBUG_INFO = 0;
    %Длинна сигнала
    L = length(data);
    fprintf("L = %d\n", L);
    Smpy = zeros(1, length(data));
    for i = 1:Nb
        if PRINT_DEBUG_INFO
            fprintf("i = %d\n", i);
        end
        tau = round((D(i) - D1) / (c * Ts));
        %fprintf("tau = %d\n", tau);
        G = c / (4 * pi * D(i) * f0);
        %k = L + round(tau);
        Si = data;
        if PRINT_DEBUG_INFO
            %fprintf("Di = %d, tau = %d, G = %f\n", D(i), tau, G);
        end
        for k = 1:(L+tau)
           if(k <= tau)
                Si(k) = 0;
           else
               Si(k) = data(k - tau);
           end
        end
        %Si = [zeros(1, round(tau)), data];
        %{
        fprintf("Si\n");
        for t = 1:length(Si)
            fprintf("%g+%gi\t", real(Si(t)), imag(Si(t)));
            if(mod(t, 8) == 0)
                fprintf("\n");
            end
        end
        %}
        Si = Si .* G;
        %Si = Si + G;
        
        %{
        fprintf("\nSi * G\n");
        for t = 1:length(Si)
            fprintf("%g+%gi ", real(Si(t)), imag(Si(t)));
            if(mod(t, 8) == 0)
                fprintf("\n");
            end
        end
        %}
        if PRINT_DEBUG_INFO
            %fprintf("len(Si) = %d\n", length(Si));
        end
        Smpy = sum_array(Smpy, Si);
        %Smpy = Smpy + Si;
        
        %Smpy = Smpy + Si;
    end
    %{
    fprintf("\nSmpy\n");
    for t = 1:length(Smpy)
        fprintf("%g+%gi ", real(Smpy(t)), imag(Smpy(t)));
        if(mod(t, 4) == 0)
            fprintf("\n");
        end
    end
    %}
    n = transpose(wgn(length(Smpy), 1, N0));
    %n = n / 500;

    if 0
        
        for t = 1:length(Smpy)
            Smpy(t) = Smpy(t) + n(t);
            Smpy(t) = Smpy(t) + (n(t) * 1i);
            
        end
    else
        %Smpy = Smpy + n + (n * 1i);
    end
    out_data = Smpy;% + n;
end




















