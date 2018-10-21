FROM rootproject/root-cc7
COPY . /rootana
WORKDIR /rootana
RUN make 
CMD source /rootana/thisrootana.sh