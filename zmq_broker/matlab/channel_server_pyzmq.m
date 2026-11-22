clear; clc;
disp('=== MATLAB 3GPP Radio Channel Server (via pyzmq) ===');

% Проверка и импорт модуля pyzmq
try
    py_zmq = py.importlib.import_module('zmq');
catch ME
    error(['Не удалось импортировать pyzmq. Убедитесь, что pyzmq установлен: ' ...
           'выполните в терминале "pip install pyzmq" для окружения Python в MATLAB.']);
end

context = py_zmq.Context();
server = context.socket(py_zmq.constants.REP);
server.bind('tcp://*:5555');

disp('MATLAB Channel Server listening on port 5555 (waiting for C++ broker)...');

sample_rate = 11.52e6;
carrier_freq = 900e6;

% Контейнер для индивидуальных моделей каналов абонентов (сохраняют фазовую историю замираний)
ue_channels = containers.Map('KeyType', 'int32', 'ValueType', 'any');

create_channel = @() nrTDLChannel(...
    'SampleRate', sample_rate, ...
    'DelayProfile', 'TDL-C', ...
    'DelaySpread', 100e-9, ...
    'MaximumDopplerShift', 15 ...
);

while true
    % 1. Читаем multipart-сообщение от C++ брокера
    % Фрейм 1: Заголовок ChannelRequestHeader (16 байт)
    % Фрейм 2: Буфер комплексных IQ-сэмплов (single precision)
    msg_parts = cell(server.recv_multipart());
    if isempty(msg_parts) || length(msg_parts) < 2
        continue;
    end
    
    hdr_raw = typecast(uint8(msg_parts{1}), 'int32');
    ue_id       = hdr_raw(1);
    direction   = hdr_raw(2); % 0 = DL, 1 = UL
    distance_km = typecast(hdr_raw(3), 'single');
    num_samples = double(hdr_raw(4));

    raw_samples = typecast(uint8(msg_parts{2}), 'single');
    iq_in = complex(raw_samples(1:2:end), raw_samples(2:2:end));

    % Инициализация модели канала для нового абонента
    if ~isKey(ue_channels, ue_id)
        ue_channels(ue_id) = create_channel();
        fprintf('[MATLAB] Initialized 3GPP TDL-C channel for UE %d\n', ue_id);
    end
    channel_obj = ue_channels(ue_id);

    % Расчет крупномасштабного затухания (Free Space / Path Loss)
    pl_db = 20*log10(double(distance_km)) + 20*log10(carrier_freq/1e6) + 32.44;
    pl_linear = 10^(-pl_db / 20);

    % Применение замираний 3GPP и затухания
    [faded_iq, ~] = channel_obj(iq_in);
    iq_out = faded_iq * pl_linear;
    iq_out = awgn(iq_out, 30, 'measured');

    % Упаковка результата [I, Q, I, Q, ...] в формат single (float32)
    out_payload = zeros(2 * num_samples, 1, 'single');
    out_payload(1:2:end) = real(iq_out);
    out_payload(2:2:end) = imag(iq_out);

    out_bytes = typecast(out_payload(:), 'uint8');

    % Отправка ответа брокеру
    server.send(py.bytes(out_bytes(:)'));
end
