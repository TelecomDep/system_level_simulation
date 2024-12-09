
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

global iter_set_dist;

iter_set_dist = 0;

global index_dl;
index_dl = 1;
global dist_list;
% dist_list = [10, 50, 100, 500, 1000, 4000];
dist_list = [10, 50, 100, 300, 500, 600, 1000];



while true
    
    if ~pauseFlag
        msg = socket_api_proxy.recv();
        if ~isempty(msg)
            % fprintf('received message [%d]\n', length(msg));
            if(length(msg) > 1000)
                % process_data(msg);
                % transmission_channel_model(msg);
                msg = simulaion(msg);
                msg = convert_to_byte(msg);
                % fprintf("send to proxe: %d\n", length(msg));
                
                % msg = complexToBytes(msg);

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


function out_data = convert_to_byte(complex_array)
    single_array = single(complex_array);
    realPart = real(single_array);
    imaginaryPart = imag(single_array);
    floatArray = zeros(1, 2 * length(single_array));
    floatArray(1:2:end) = realPart;    
    floatArray(2:2:end) = imaginaryPart; 
    out_data = typecast(single(floatArray), 'uint8');
    
end

function out_data = CostHata(data, h_enb, h_ue, d)

    fc = 2560; % Частота в МГц
    hte = h_enb; % Высота передающей антенны в метрах
    hre = h_ue; % Высота приемной антенны в метрах
    %d = 5; % Расстояние между передатчиком и приемником в километрах
    Cm = 0; % Поправочный коэффициент для средних городов и пригородов
    
    % Расчет поправочного коэффициента для высоты приемной антенны
    a_hre = (1.1 * log10(fc) - 0.7) * hre - (1.56 * log10(fc) - 0.8);
    
    % Расчет потерь сигнала
    L = 46.3 + 33.9 * log10(fc) - 13.82 * log10(hte) - a_hre + (44.9 - 6.55 * log10(hte)) * log10(d) + Cm;
    %fprintf("L = %f\n", L);
    out_data = data - L;
end


function out_data = simulaion(data_raw)
    

    global iter_set_dist;
    global index_dl;
    global dist_list;
        
    data_slice = data_raw;
    floatArray = typecast(data_slice, 'single');
    
    data = complex(floatArray(1:2:end), floatArray(2:2:end));
    % fprintf("size data = %d\n", length(data));
    CON = 2;
    
    pos_ENB = [100, 200, 50];
    pos_UE = [200, 200, 1.5];
    
    
    iter_set_dist = iter_set_dist + 1;
    distance = 1;
    if iter_set_dist > 1000


        distance = dist_list(index_dl);
        fprintf("new distance: %d, index: %d\n", distance, index_dl);
        index_dl = index_dl + 1;

        if index_dl > length(dist_list)
            index_dl = 1;
            
        end
        iter_set_dist = 0;
    end

    %distance = 400;


    if(CON == 1)
        distance = sqrt((pos_UE(1) - pos_ENB(1))^2 + (pos_UE(2) - pos_ENB(2))^2);
    end

    %fprintf("dist = %f\n", distance);


    mu = 0; % Среднее значение
    sigma = distance; % Стандартное отклонение
    n = length(data); % Количество точек
    
    % Генерация нормально распределённого шума
    noise = mu + sigma * randn(n, 1);
    
    % Ограничение шума до ±100
    noise = max(min(noise, 100), -100);
    noise = noise + noise * 1i;

    % data_cost = CostHata(data, pos_ENB(3), pos_UE(3), distance);
    data_cost = data / (distance / 10) + noise;

    out_data = data_cost;
    
    % out_data = data;


    %fprintf("data_cost = %f\n", data_cost);
end

function recovered_data_raw = complexToBytes(complexArray)
    % Преобразует массив комплексных чисел обратно в массив байтов
    % complexArray: Входной массив комплексных чисел
    % recovered_data_raw: Выходной массив байтов (uint8)
    
    % Извлечение реальных и мнимых частей
    recovered_floatArray = [real(complexArray); imag(complexArray)];
    
    % Объединение в одномерный массив
    recovered_floatArray = recovered_floatArray(:); % Приводим к одномерному массиву
    
    % Преобразование обратно в байтовый массив
    recovered_data_raw = typecast(recovered_floatArray, 'uint8');
end


function out_data = transmission_channel_model(data_raw)

    data_slice = data_raw;
    floatArray = typecast(uint8(data_slice), 'single');
    complexArray = complex(floatArray(1:2:end), floatArray(2:2:end));

    %out_data = 
end



function out_data = transmission_channel_model0(data, c, Nb, f0, Ts, D1, Dn, N0)
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




















