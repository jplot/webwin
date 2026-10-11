// The page's transport for net_threads: every datagram comes straight back.
Module['webnetDropped'] = 0;
Module['webnet'] = {
  send: (address, port, bytes) => {
    if (!Module['webnetDeliver'](address, port, bytes)) Module['webnetDropped']++;
  },
};
