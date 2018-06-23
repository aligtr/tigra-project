function real_time_plot
    global dat
    delete(instrfind);
    dat = serial('COM5', 'BaudRate', 115200);
    dat.InputBufferSize = 4096;
    fopen(dat)
    set(dat, 'ByteOrder', 'littleEndian')
    
    fwrite(dat, 1, 'uint8');
    disp 'Ok!'
    A=[];
    B = [];
    
    vector = [0];
    num = 0;
    
    f=figure('visible','off','position',...
        [0 0 640 480]);
    slhan=uicontrol('style','slider', 'units','normalized','position',[0 1-.05 1 .05],...
        'min',0,'max',10,'callback',@callbackfn);
    hsttext=uicontrol('style','text', 'units','normalized',...
        'position',[0 1-.05-.07 1 .05]);
    set(hsttext,'visible','on','string',num2str(num))
    axes('units','normalized','position',[.05 .05 .9 .9-.05-.05], 'PlotBoxAspectRatioMode', 'auto');
    movegui(f,'center')
    set(f,'visible','on');
    
    function callbackfn(source,eventdata)
        num=get(slhan,'value');
        set(hsttext,'visible','on','string',num2str(num))
    end

    point_number = 1000;
    while length(vector) < point_number
		res = fread(dat, 1, 'uint16');
	
        vector = [vector res];
        
        plot(gca, vector); drawnow limitrate
        
    end
	
    fwrite(dat, 2, 'uint8');
    fclose(dat);
    disp 'Finish'
end

