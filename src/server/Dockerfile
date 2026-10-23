FROM ghcr.io/astral-sh/uv:0.9.27 AS uv

FROM python:3.12-slim AS build
COPY --from=uv /uv /bin/uv
WORKDIR /app
ENV UV_LINK_MODE=copy UV_COMPILE_BYTECODE=1 UV_PYTHON_DOWNLOADS=never
COPY pyproject.toml uv.lock ./
COPY espresso_api ./espresso_api
RUN uv sync --frozen --no-dev --no-editable

FROM python:3.12-slim
WORKDIR /app
ENV PATH="/app/.venv/bin:$PATH" \
    ESPRESSO_DB_PATH="/data/espresso.sqlite3" \
    PYTHONUNBUFFERED=1
RUN useradd --uid 10001 --create-home espresso \
    && mkdir /data && chown espresso:espresso /data
COPY --from=build /app/.venv /app/.venv
USER espresso
EXPOSE 8000
HEALTHCHECK --interval=30s --timeout=5s --start-period=10s \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/api/v1/health', timeout=3)"
CMD ["uvicorn", "espresso_api.app:app", "--host", "0.0.0.0", "--port", "8000", "--workers", "1"]
