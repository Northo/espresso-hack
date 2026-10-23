import asyncio

from espresso_api.middleware import BodyLimitMiddleware


def test_chunked_body_limit_does_not_trust_content_length():
    async def exercise():
        messages = iter(
            [
                {"type": "http.request", "body": b"123", "more_body": True},
                {"type": "http.request", "body": b"456", "more_body": False},
            ]
        )
        sent = []

        async def receive():
            return next(messages)

        async def send(message):
            sent.append(message)

        async def downstream(scope, receive, send):
            raise AssertionError("Oversized request must not reach the application")

        middleware = BodyLimitMiddleware(downstream, max_bytes=5)
        await middleware(
            {"type": "http", "method": "POST", "headers": [(b"content-length", b"1")]},
            receive,
            send,
        )
        assert sent[0]["status"] == 413

    asyncio.run(exercise())


def test_buffered_body_is_replayed_once_then_disconnect_is_forwarded():
    async def exercise():
        messages = iter(
            [
                {"type": "http.request", "body": b"12", "more_body": True},
                {"type": "http.request", "body": b"34", "more_body": False},
                {"type": "http.disconnect"},
            ]
        )

        async def receive():
            return next(messages)

        async def send(message):
            pass

        async def downstream(scope, receive, send):
            assert await receive() == {
                "type": "http.request",
                "body": b"1234",
                "more_body": False,
            }
            assert await receive() == {"type": "http.disconnect"}

        await BodyLimitMiddleware(downstream)(
            {"type": "http", "method": "POST"},
            receive,
            send,
        )

    asyncio.run(exercise())
