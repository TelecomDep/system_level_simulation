server = tcpclient("localhost", 4000);
logFile = fopen("/home/sceptik/Desktop/recv_log.txt", "w");

gnbs = 1;
ues = 2;

cur_sum_id = 0;
target_sum_ids = gnbs + ues;

packets_sizes = zeros(1, gnbs + ues, 'int32');

packets = cell(1, length(packets_sizes));

if logFile == -1
    error("File not open");
end

while true

    if server.NumBytesAvailable > 0

        rcv_packets = 0;

        rcv_packets_sizes = read(server, 4 * (gnbs + ues), "uint8");

        if length(rcv_packets_sizes) ~= (gnbs + ues) * 4
            fprintf("\nInvalid packet sizes\n");
            continue;
        end

        fprintf("\nMATLAB receive packet sizes \t size = %d\n", length(rcv_packets_sizes));

        for i = 1 : numel(rcv_packets_sizes) / 4
            packets_sizes(i) = typecast(rcv_packets_sizes(4 * (i - 1) + 1 : 4 * i), "int32");
            fprintf("\nID = %d \t packet size = %d\n", i - 1, packets_sizes(i));
        end
         pause(0.3);
         rcv_packets = read(server, server.NumBytesAvailable, "uint8");
         
         if length(rcv_packets) ~= sum(packets_sizes) + gnbs + ues
             fprintf("\nMATLAB receive invalid packet \t size = %d \t expected size = %d\n", length(rcv_packets), sum(packets_sizes) + gnbs + ues);
             continue;
         end

         fprintf("\nMATLAB receive packet \t size = %d\n", length(rcv_packets));

         for i = 1 : length(packets_sizes)
            if i == 1
                start = 1;
            else
                start = sum(packets_sizes(1 : i - 1)) + (i - 1) + 1; 
            end

            End = sum(packets_sizes(1 : i)) + i;  
            
            ID = rcv_packets(start);
            
            packet_data = rcv_packets(start + 1 : End);
            
            packets{i} = packet_data;
            fprintf("ID = %d \t packet size = %d bytes (expected %d)\n", ...
                ID, numel(packet_data), packets_sizes(i));
         end

         write(server, rcv_packets, "uint8");
         
    end

end