clear; clc;
disp('=== MATLAB 3GPP Radio Channel Server ===');

server = zeromq("rep", "LocalPort", 5555);
disp('Waiting for broker connections on port 5555...');

sample_rate = 11.52e6;
carrier_freq = 900e6;

% Map to store individual 3GPP channel objects per UE
ue_channels = containers.Map('KeyType', 'int32', 'ValueType', 'any');

create_channel = @() nrTDLChannel(...
    'SampleRate', sample_rate, ...
    'DelayProfile', 'TDL-C', ...
    'DelaySpread', 100e-9, ...
    'MaximumDopplerShift', 15 ...
);

while true
    % Frame 1: Header metadata
    raw_header = read(server, "int32");
    if isempty(raw_header), continue; end
    
    ue_id       = raw_header(1);
    direction   = raw_header(2); % 0 = DL, 1 = UL
    distance_km = typecast(raw_header(3), 'single');
    num_samples = raw_header(4);

    % Frame 2: IQ samples
    raw_samples = read(server, "single");
    iq_in = complex(raw_samples(1:2:end), raw_samples(2:2:end));

    if ~isKey(ue_channels, ue_id)
        ue_channels(ue_id) = create_channel();
        disp(['[MATLAB] Created 3GPP channel for UE ', num2str(ue_id)]);
    end
    channel_obj = ue_channels(ue_id);

    % Path loss
    pl_db = 20*log10(distance_km) + 20*log10(carrier_freq/1e6) + 32.44;
    pl_linear = 10^(-pl_db / 20);

    % Fading and AWGN
    [faded_iq, ~] = channel_obj(iq_in);
    iq_out = faded_iq * pl_linear;
    iq_out = awgn(iq_out, 30, 'measured');

    % Return payload [I, Q, I, Q...]
    out_payload = zeros(2 * num_samples, 1, 'single');
    out_payload(1:2:end) = real(iq_out);
    out_payload(2:2:end) = imag(iq_out);

    write(server, out_payload);
end
