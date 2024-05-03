-module(comp).

-export([comp/1, comp/2, comp_proc/2, comp_proc/3, comp_loop_procs/3, decomp/1, decomp/2]).

-define(DEFAULT_CHUNK_SIZE, 1024*1024).


process_Killer(ProcList,Reader,Writer) ->
    case ProcList of
        [H | T] -> 
            receive
                {done,H} -> process_Killer(T,Reader,Writer);
                {error,_} -> exit
            end;
        [] -> 
            Reader ! stop,
            Writer ! stop
    end.

proc_spawn_loop(0, _ ,Reader, Writer, ProcList) -> 
    process_Killer(ProcList,Reader,Writer);
proc_spawn_loop(N,Foo,Reader,Writer,ProcList) when N > 0 ->
    Pid = spawn_link(comp,Foo,[Reader,Writer,self()]),
    proc_spawn_loop(N-1,Foo,Reader,Writer,[Pid | ProcList] ).



%%% File Compression

comp_loop(Reader, Writer) ->  %% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop(Reader, Writer);
        eof ->  %% end of file, stop reader and writer
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.

comp_loop_procs(Reader, Writer, MainProc) ->  %% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   %% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop_procs(Reader, Writer, MainProc);
        eof ->  %% end of file
            MainProc ! {done, self()};
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            MainProc ! {error, Reason}  
            
    end.

comp(File) -> %% Compress file to file.ch
    comp(File, ?DEFAULT_CHUNK_SIZE).

comp(File, Chunk_Size) ->  %% Starts a reader and a writer which run in separate processes
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    comp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.


comp_proc(File, Procs) ->
    comp_proc(File, ?DEFAULT_CHUNK_SIZE, Procs).

comp_proc(File, Chunk_Size, Procs) ->
        case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    proc_spawn_loop(Procs,comp_loop_procs,Reader,Writer,[]);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.





%% File Decompression

decomp_loop(Reader, Writer) ->
    Reader ! {get_chunk, self()},  %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop(Reader, Writer);
        eof ->    %% end of file => exit decompression
            Reader ! stop,
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.

decomp(Archive) ->
    decomp(Archive, string:replace(Archive, ".ch", "", trailing)).

decomp(Archive, Output_File) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    decomp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.
