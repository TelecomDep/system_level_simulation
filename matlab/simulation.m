





data = [2, 3, 2, 4, 5];

simulaion(data);

complex_array = [1 + 2i, 3 + 4i, 5 + 6i];
%byte_array = typecast(complex_array, 'uint8');

single_array = single(complex_array);

real_parts = real(single_array);
imaginary_parts = imag(single_array);

combined_array = reshape([real_parts; imaginary_parts], 1, []);
byte_array = typecast(combined_array, 'uint8');
fprintf("%d\n", length(byte_array));



recovered_array = typecast(uint8(byte_array), 'single');

reconstructed_complex = complex(recovered_array(1:2:end), recovered_array(2:2:end));

reconstructed_complex

"=============="
recovered_data_raw = complexToBytes(complex_array);
disp('Восстановленный байтовый массив:');
disp(recovered_data_raw);


complexArray = bytesToComplex(recovered_data_raw);
disp('Комплексный массив:');
disp(complexArray);


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

function simulaion(data)

    CON = 2;
    
    pos_ENB = [100, 200, 50];
    pos_UE = [200, 200, 1.5];
    
    distance = 10;


    if(CON == 1)
        distance = sqrt((pos_UE(1) - pos_ENB(1))^2 + (pos_UE(2) - pos_ENB(2))^2);
    end

    %fprintf("dist = %f\n", distance);

    data_cost = CostHata(data, pos_ENB(3), pos_UE(3), distance);

    %fprintf("data_cost = %f\n", data_cost);
end

function out_data = convert_to_byte(complex_array)
    single_array = single(complex_array);
    real_parts = real(single_array);
    imaginary_parts = imag(single_array);
    combined_array = reshape([real_parts; imaginary_parts], 1, []);
    out_data = typecast(combined_array, 'uint8');
    
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
function complexArray = bytesToComplex(data_raw)
    % Преобразует массив байтов в массив комплексных чисел
    % data_raw: Входной массив байтов (uint8)
    % complexArray: Выходной массив комплексных чисел
    
    % Преобразование байтов в массив типа single
    floatArray = typecast(uint8(data_raw), 'single');
    
    % Создание массива комплексных чисел
    complexArray = complex(floatArray(1:2:end), floatArray(2:2:end));
end
