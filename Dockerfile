FROM rootproject/root-cc7
COPY . /rootana
RUN make /rootana
CMD source /rootana/thisrootana