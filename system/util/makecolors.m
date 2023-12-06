l = 1024;
c = ind2rgb([1:l], gray(l));
rgba = round(c(1, :, 1) * 255);
rgba = rgba + bitshift(round(c(1, :, 2) * 255), 8);
rgba = rgba + bitshift(round(c(1, :, 3) * 255), 16);
rgba = rgba + repmat(bitshift(255, 24), 1, l);


f = fopen('colormap.txt', 'w');
fwrite(f, '{');
for i = 1:l
    fprintf(f, '0x%08X', rgba(i));
    if i ~= l
       fwrite(f, ', '); 
    end
end
fwrite(f, '};');
fclose(f);
