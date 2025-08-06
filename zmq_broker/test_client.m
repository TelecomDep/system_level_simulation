server = tcpclient("localhost", 4000);
logFile = fopen("/home/sceptik/Desktop/recv_log.txt", "w");

if logFile == -1
    error("File not open");
end

write(server, "Hello", "string");

while true
  
    if server.NumBytesAvailable > 0
        data = read(server, server.NumBytesAvailable, "uint8");
    
        first_byte = int8(data(1));

        if mod(length(data(2:end)), 2) ~= 0
            continue;
        end
    
        floatArray = typecast(data(2:end), 'single');
    
        if mod(length(floatArray), 2) ~= 0
            disp("Invalid data");
            continue;
        end
    
        realParts = floatArray(1:2:end);
        imagParts = floatArray(2:2:end);
        complexArray = complex(realParts, imagParts);
    
        re = real(complexArray);
        im = imag(complexArray);
        floatArray_out = zeros(1, numel(re) + numel(im), 'single');
        floatArray_out(1:2:end) = re;
        floatArray_out(2:2:end) = im;
    
        byteArray_out = typecast(floatArray_out, 'uint8');
        %write(server, byteArray_out, "uint8");
    
        if first_byte < 0
            fprintf(logFile, 'DATA FROM GNB_%d:\n%.4f%+.4fi\n', abs(first_byte) - 1, [real(complexArray); imag(complexArray)]);
            fprintf("\n");
        else
            fprintf(logFile, 'DATA FROM UE_%d:\n%.4f%+.4fi\n', first_byte, [real(complexArray); imag(complexArray)]);
            fprintf("\n");
        end
    end
end